/*
* Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Map.h"
#include "MapManager.h"
#include "Group.h"
#include "WorldPacket.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "CreatureTextMgr.h"
#include "GroupMgr.h"

Battlefield::Battlefield()
{
    m_Timer                     = 0;
    m_IsEnabled                 = true;
    m_isActive                  = false;
    m_DefenderTeam              = TEAM_NEUTRAL;

    m_Guid                      = 0;
    m_TypeId                    = 0;
    m_BattleId                  = 0;
    m_ZoneId                    = 0;
    m_MapId                     = 0;
    m_MaxPlayer                 = 0;
    m_MinPlayer                 = 0;
    m_BattleTime                = 0;
    m_NoWarBattleTime           = 0;
    m_TimeForAcceptInvite       = 20;

    m_uiKickAfkPlayersTimer     = 1000;

    m_LastResurectTimer         = 30 * IN_MILLISECONDS;
    m_StartGroupingTimer        = 0;
    m_StartGrouping             = false;
    StalkerGuid                 = 0;
    m_TowerBonus                = 0;
}

Battlefield::~Battlefield()
{
    for (BfCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
        delete itr->second;

    for (GraveyardVect::const_iterator itr = m_GraveyardList.begin(); itr != m_GraveyardList.end(); ++itr)
        delete *itr;

    m_capturePoints.clear();
}

void Battlefield::HandlePlayerEnterZone(Player* player, uint32 /*zone*/)
{
    if (IsWarTime())
    {
        if (m_PlayersInWar[player->GetTeamId()].size() + m_InvitedPlayers[player->GetTeamId()].size() < m_MaxPlayer)
            InvitePlayerToWar(player);
        else
        {
            // TODO: Send a packet to announce it to player
            m_PlayersWillBeKick[player->GetTeamId()][player->GetGUID()] = time(NULL) + 10;
            InvitePlayerToQueue(player);
        }
    }
    else
    {
        if (m_Timer <= m_StartGroupingTimer)
            InvitePlayerToQueue(player);
    }

    m_players[player->GetTeamId()].insert(player->GetGUID());
    OnPlayerEnterZone(player);
}

void Battlefield::HandlePlayerLeaveZone(Player* player, uint32 /*zone*/)
{
    if (IsWarTime())
    {
        if (m_PlayersInWar[player->GetTeamId()].find(player->GetGUID()) != m_PlayersInWar[player->GetTeamId()].end())
        {
            m_PlayersInWar[player->GetTeamId()].erase(player->GetGUID());
            player->GetSession()->SendBfLeaveMessage(m_Guid);
            if (Group* group = player->GetGroup())
                group->RemoveMember(player->GetGUID());

            OnPlayerLeaveWar(player);
        }
    }

    for (BfCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
        itr->second->HandlePlayerLeave(player);

    m_InvitedPlayers[player->GetTeamId()].erase(player->GetGUID());
    m_PlayersWillBeKick[player->GetTeamId()].erase(player->GetGUID());
    m_players[player->GetTeamId()].erase(player->GetGUID());
    SendRemoveWorldStates(player);
    RemovePlayerFromResurrectQueue(player->GetGUID());
    OnPlayerLeaveZone(player);
}

bool Battlefield::Update(uint32 diff)
{
    if (m_Timer <= diff)
    {
        if (IsWarTime())
            EndBattle(true);
        else
            StartBattle();
    }
    else
        m_Timer -= diff;

    if (!IsWarTime() && !m_StartGrouping && m_Timer <= m_StartGroupingTimer)
    {
        m_StartGrouping = true;
        InvitePlayersInZoneToQueue();
        OnStartGrouping();
    }

    bool objective_changed = false;
    if (IsWarTime())
    {
        if (m_uiKickAfkPlayersTimer <= diff)
        {
            m_uiKickAfkPlayersTimer = 1000;
            KickAfkPlayers();
        }
        else
            m_uiKickAfkPlayersTimer -= diff;

        for (BfCapturePointMap::iterator itr = m_capturePoints.begin(); itr != m_capturePoints.end(); ++itr)
            if (itr->second->Update(diff))
                objective_changed = true;
    }


    if (m_LastResurectTimer <= diff)
    {
        for (uint8 i = 0; i < m_GraveyardList.size(); i++)
            if (GetGraveyardById(i))
                m_GraveyardList[i]->Resurrect();
        m_LastResurectTimer = RESURRECTION_INTERVAL;
    }
    else
        m_LastResurectTimer -= diff;

    return objective_changed;
}

void Battlefield::InvitePlayersInZoneToQueue()
{
    for (uint8 team = 0; team < 2; ++team)
        for (GuidSet::const_iterator itr = m_players[team].begin(); itr != m_players[team].end(); ++itr)
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
                InvitePlayerToQueue(player);
}

void Battlefield::InvitePlayerToQueue(Player* player)
{
    if (m_PlayersInQueue[player->GetTeamId()].count(player->GetGUID()))
        return;

    if (m_PlayersInQueue[player->GetTeamId()].size() <= m_MinPlayer || m_PlayersInQueue[GetOtherTeam(player->GetTeamId())].size() >= m_MinPlayer)
        player->GetSession()->SendBfInvitePlayerToQueue(m_Guid);
}

void Battlefield::InvitePlayersInQueueToWar()
{
    for (uint8 team = 0; team < BG_TEAMS_COUNT; ++team)
    {
        for (GuidSet::const_iterator itr = m_PlayersInQueue[team].begin(); itr != m_PlayersInQueue[team].end(); ++itr)
        {
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
            {
                if (m_PlayersInWar[player->GetTeamId()].size() + m_InvitedPlayers[player->GetTeamId()].size() < m_MaxPlayer)
                    InvitePlayerToWar(player);
                else
                {
                    //Full
                }
            }
        }
        m_PlayersInQueue[team].clear();
    }
}

void Battlefield::InvitePlayersInZoneToWar()
{
    for (uint8 team = 0; team < BG_TEAMS_COUNT; ++team)
        for (GuidSet::const_iterator itr = m_players[team].begin(); itr != m_players[team].end(); ++itr)
        {
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
            {
                if (m_PlayersInWar[player->GetTeamId()].count(player->GetGUID()) || m_InvitedPlayers[player->GetTeamId()].count(player->GetGUID()))
                    continue;
                if (m_PlayersInWar[player->GetTeamId()].size() + m_InvitedPlayers[player->GetTeamId()].size() < m_MaxPlayer)
                    InvitePlayerToWar(player);
                else
                    m_PlayersWillBeKick[player->GetTeamId()][player->GetGUID()] = time(NULL) + 10;
            }
        }
}

void Battlefield::InvitePlayerToWar(Player* player)
{
    if (!player)
        return;

    if (player->InArena() || player->GetBattleground())
    {
        m_PlayersInQueue[player->GetTeamId()].erase(player->GetGUID());
        return;
    }

    if (player->getLevel() < m_MinLevel)
    {
        if (m_PlayersWillBeKick[player->GetTeamId()].count(player->GetGUID()) == 0)
            m_PlayersWillBeKick[player->GetTeamId()][player->GetGUID()] = time(NULL) + 10;
        return;
    }

    if (m_PlayersInWar[player->GetTeamId()].count(player->GetGUID()) || m_InvitedPlayers[player->GetTeamId()].count(player->GetGUID()))
        return;

    m_PlayersWillBeKick[player->GetTeamId()].erase(player->GetGUID());
    m_InvitedPlayers[player->GetTeamId()][player->GetGUID()] = time(NULL) + m_TimeForAcceptInvite;
    player->GetSession()->SendBfInvitePlayerToWar(m_Guid, m_ZoneId, m_TimeForAcceptInvite);
}

void Battlefield::InitStalker(uint32 entry, float x, float y, float z, float o)
{
    if (Creature* creature = SpawnCreature(entry, x, y, z, o, TEAM_NEUTRAL))
        StalkerGuid = creature->GetGUID();
    else
        sLog->outError(LOG_FILTER_BATTLEFIELD, "Battlefield::InitStalker: could not spawn Stalker (Creature entry %u), zone messeges will be un-available", entry);
}

void Battlefield::KickAfkPlayers()
{
    for (uint8 team = 0; team < BG_TEAMS_COUNT; ++team)
        for (GuidSet::const_iterator itr = m_PlayersInWar[team].begin(); itr != m_PlayersInWar[team].end(); ++itr)
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
                if (player->isAFK())
                    KickPlayerFromBattlefield(*itr);
}

void Battlefield::KickPlayerFromBattlefield(uint64 guid)
{
    if (Player* player = sObjectAccessor->FindPlayer(guid))
        if (player->GetZoneId() == GetZoneId())
            player->TeleportTo(KickPosition);
}

void Battlefield::StartBattle()
{
    if (m_isActive)
        return;

    for (int team = 0; team < BG_TEAMS_COUNT; team++)
    {
        m_PlayersInWar[team].clear();
        m_Groups[team].clear();
    }

    m_Timer = m_BattleTime;
    m_isActive = true;

    InvitePlayersInZoneToWar();
    InvitePlayersInQueueToWar();

    DoPlaySoundToAll(BF_START);

    OnBattleStart();
}

void Battlefield::EndBattle(bool endByTimer)
{
    if (!m_isActive)
        return;

    m_isActive = false;

    m_StartGrouping = false;

    if (!endByTimer)
        SetDefenderTeam(GetAttackerTeam());

    if (GetDefenderTeam() == TEAM_ALLIANCE)
        DoPlaySoundToAll(BF_ALLIANCE_WINS);
    else
        DoPlaySoundToAll(BF_HORDE_WINS);

    OnBattleEnd(endByTimer);

    m_Timer = m_NoWarBattleTime;
    SendInitWorldStatesToAll();
}

void Battlefield::DoPlaySoundToAll(uint32 SoundID)
{
    for (int team = 0; team < BG_TEAMS_COUNT; team++)
    {
        for (GuidSet::const_iterator itr = m_PlayersInWar[team].begin(); itr != m_PlayersInWar[team].end(); ++itr)
        {
            uint64 guid = (*itr);
            if (Player* player = sObjectAccessor->FindPlayer(guid))
                player->SendSound(SoundID, guid);
        }
    }
}

bool Battlefield::HasPlayer(Player* player) const
{
    return m_players[player->GetTeamId()].find(player->GetGUID()) != m_players[player->GetTeamId()].end();
}

void Battlefield::PlayerAcceptInviteToQueue(Player* player)
{
    m_PlayersInQueue[player->GetTeamId()].insert(player->GetGUID());
    player->GetSession()->SendBfQueueInviteResponse(m_Guid, m_ZoneId);
}

void Battlefield::AskToLeaveQueue(Player* player)
{
    m_PlayersInQueue[player->GetTeamId()].erase(player->GetGUID());
}

void Battlefield::PlayerAcceptInviteToWar(Player* player)
{
    if (!IsWarTime())
        return;

    if (AddOrSetPlayerToCorrectBfGroup(player))
    {
        player->GetSession()->SendBfEntered(m_Guid);
        m_PlayersInWar[player->GetTeamId()].insert(player->GetGUID());
        m_InvitedPlayers[player->GetTeamId()].erase(player->GetGUID());

        if (player->isAFK())
            player->ToggleAFK();

        OnPlayerJoinWar(player);
    }
}

void Battlefield::TeamCastSpell(TeamId team, int32 spellId)
{
    if (spellId > 0)
        for (GuidSet::const_iterator itr = m_PlayersInWar[team].begin(); itr != m_PlayersInWar[team].end(); ++itr)
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
                player->CastSpell(player, uint32(spellId), true);
            else
                for (GuidSet::const_iterator itr = m_PlayersInWar[team].begin(); itr != m_PlayersInWar[team].end(); ++itr)
                    if (Player* player = sObjectAccessor->FindPlayer(*itr))
                        player->RemoveAuraFromStack(uint32(-spellId));
}

void Battlefield::BroadcastPacketToZone(WorldPacket& data) const
{
    for (uint8 team = 0; team < BG_TEAMS_COUNT; ++team)
        for (GuidSet::const_iterator itr = m_players[team].begin(); itr != m_players[team].end(); ++itr)
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
                player->GetSession()->SendPacket(&data);
}

void Battlefield::BroadcastPacketToQueue(WorldPacket& data) const
{
    for (uint8 team = 0; team < BG_TEAMS_COUNT; ++team)
        for (GuidSet::const_iterator itr = m_PlayersInQueue[team].begin(); itr != m_PlayersInQueue[team].end(); ++itr)
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
                player->GetSession()->SendPacket(&data);
}

void Battlefield::BroadcastPacketToWar(WorldPacket& data) const
{
    for (uint8 team = 0; team < BG_TEAMS_COUNT; ++team)
        for (GuidSet::const_iterator itr = m_PlayersInWar[team].begin(); itr != m_PlayersInWar[team].end(); ++itr)
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
                player->GetSession()->SendPacket(&data);
}

WorldPacket Battlefield::BuildWarningAnnPacket(std::string msg)
{
    WorldPacket data(SMSG_MESSAGECHAT, 40 + msg.length() + 1);

    data << uint8(CHAT_MSG_RAID_BOSS_EMOTE);
    data << uint32(LANG_UNIVERSAL);
    data << uint64(0);
    data << uint32(0);                                      // 2.1.0
    data << uint32(1);
    data << uint8(0);
    data << uint64(0);
    data << uint32(msg.length() + 1);
    data << msg;
    data << uint8(0);
    data << float(0);
    data << uint8(0);

    return data;
}

void Battlefield::SendWarningToAllInZone(uint32 entry,...)
{
    const char *format = sObjectMgr->GetTrinityStringForDBCLocale(entry);
    va_list ap;
    char str [1024];
    va_start(ap, entry);
    vsnprintf(str, 1024, format, ap);
    va_end(ap);
    std::string msg = (std::string)str;
    WorldPacket data = BuildWarningAnnPacket(msg);
    BroadcastPacketToZone(data);
}

void Battlefield::SendWarningToPlayer(Player* player, uint32 entry)
{
    if (player)
        if (Unit* unit = sObjectAccessor->FindUnit(StalkerGuid))
            if (Creature* stalker = unit->ToCreature())
                sCreatureTextMgr->SendChat(stalker, (uint8)entry, player->GetGUID());
}

void Battlefield::SendUpdateWorldState(uint32 field, uint32 value)
{
    for (uint8 i = 0; i < BG_TEAMS_COUNT; ++i)
        for (GuidSet::iterator itr = m_players[i].begin(); itr != m_players[i].end(); ++itr)
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
                player->SendUpdateWorldState(field, value);
}

void Battlefield::RegisterZone(uint32 zoneId)
{
    sBattlefieldMgr->AddZone(zoneId, this);
}

void Battlefield::HideNpc(Creature* creature)
{
    creature->CombatStop();
    creature->SetReactState(REACT_PASSIVE);
    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
    creature->SetPhaseMask(2, true);
    creature->DisappearAndDie();
    creature->SetVisible(false);
}

void Battlefield::ShowNpc(Creature* creature, bool aggressive)
{
    creature->SetPhaseMask(1, true);
    creature->SetVisible(true);
    creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
    if (!creature->isAlive())
        creature->Respawn(true);
    if (aggressive)
        creature->SetReactState(REACT_AGGRESSIVE);
    else
    {
        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        creature->SetReactState(REACT_PASSIVE);
    }
}

// ****************************************************
// ******************* Group System *******************
// ****************************************************
Group* Battlefield::GetFreeBfRaid(TeamId TeamId)
{
    for (GuidSet::const_iterator itr = m_Groups[TeamId].begin(); itr != m_Groups[TeamId].end(); ++itr)
        if (Group* group = sGroupMgr->GetGroupByGUID(*itr))
            if (!group->IsFull())
                return group;

    return NULL;
}

Group* Battlefield::GetGroupPlayer(uint64 guid, TeamId TeamId)
{
    for (GuidSet::const_iterator itr = m_Groups[TeamId].begin(); itr != m_Groups[TeamId].end(); ++itr)
        if (Group* group = sGroupMgr->GetGroupByGUID(*itr))
            if (group->IsMember(guid))
                return group;

    return NULL;
}

bool Battlefield::AddOrSetPlayerToCorrectBfGroup(Player* player)
{
    if (!player->IsInWorld())
        return false;

    if (Group* group = player->GetGroup())
        group->RemoveMember(player->GetGUID());

    Group* group = GetFreeBfRaid(player->GetTeamId());
    if (!group)
    {
        group = new Group;
        group->SetBattlefieldGroup(this);
        group->Create(player);
        sGroupMgr->AddGroup(group);
        m_Groups[player->GetTeamId()].insert(group->GetGUID());
    }
    else if (group->IsMember(player->GetGUID()))
    {
        uint8 subgroup = group->GetMemberGroup(player->GetGUID());
        player->SetBattlegroundOrBattlefieldRaid(group, subgroup);
    }
    else
        group->AddMember(player);

    return true;
}

//***************End of Group System*******************

//*****************************************************
//***************Spirit Guide System*******************
//*****************************************************

BfGraveyard* Battlefield::GetGraveyardById(uint32 id)
{
    if (id < m_GraveyardList.size())
    {
        if (m_GraveyardList[id])
            return m_GraveyardList[id];
        else
            sLog->outError(LOG_FILTER_BATTLEFIELD, "Battlefield::GetGraveyardById Id:%u not existed", id);
    }
    else
        sLog->outError(LOG_FILTER_BATTLEFIELD, "Battlefield::GetGraveyardById Id:%u cant be found", id);

    return NULL;
}

WorldSafeLocsEntry const * Battlefield::GetClosestGraveYard(Player* player)
{
    BfGraveyard* closestGY = NULL;
    float maxdist = -1;
    for (uint8 i = 0; i < m_GraveyardList.size(); i++)
    {
        if (m_GraveyardList[i])
        {
            if (m_GraveyardList[i]->GetControlTeamId() != player->GetTeamId())
                continue;

            float dist = m_GraveyardList[i]->GetDistance(player);
            if (dist < maxdist || maxdist < 0)
            {
                closestGY = m_GraveyardList[i];
                maxdist = dist;
            }
        }
    }

    if (closestGY)
        return sWorldSafeLocsStore.LookupEntry(closestGY->GetGraveyardId());

    return NULL;
}

void Battlefield::AddPlayerToResurrectQueue(uint64 npcGuid, uint64 playerGuid)
{
    for (uint8 i = 0; i < m_GraveyardList.size(); i++)
    {
        if (!m_GraveyardList[i])
            continue;

        if (m_GraveyardList[i]->HasNpc(npcGuid))
        {
            m_GraveyardList[i]->AddPlayer(playerGuid);
            break;
        }
    }
}

void Battlefield::RemovePlayerFromResurrectQueue(uint64 playerGuid)
{
    for (uint8 i = 0; i < m_GraveyardList.size(); i++)
    {
        if (!m_GraveyardList[i])
            continue;

        if (m_GraveyardList[i]->HasPlayer(playerGuid))
        {
            m_GraveyardList[i]->RemovePlayer(playerGuid);
            break;
        }
    }
}

void Battlefield::SendAreaSpiritHealerQueryOpcode(Player* player, uint64 guid)
{
    WorldPacket data(SMSG_AREA_SPIRIT_HEALER_TIME, 12);
    uint32 time = m_LastResurectTimer;

    data << guid << time;
    ASSERT(player && player->GetSession());
    player->GetSession()->SendPacket(&data);
}

BfGraveyard::BfGraveyard(Battlefield* battlefield)
{
    m_Bf = battlefield;
    m_GraveyardId = 0;
    m_ControlTeam = TEAM_NEUTRAL;
    m_SpiritGuide[0] = 0;
    m_SpiritGuide[1] = 0;
    m_ResurrectQueue.clear();
}

void BfGraveyard::Initialize(TeamId startControl, uint32 graveyardId)
{
    m_ControlTeam = startControl;
    m_GraveyardId = graveyardId;
}

void BfGraveyard::SetSpirit(Creature* spirit, TeamId team)
{
    if (!spirit)
    {
        sLog->outError(LOG_FILTER_BATTLEFIELD, "BfGraveyard::SetSpirit: Invalid Spirit.");
        return;
    }

    m_SpiritGuide[team] = spirit->GetGUID();
    spirit->SetReactState(REACT_PASSIVE);
}

float BfGraveyard::GetDistance(Player* player)
{
    const WorldSafeLocsEntry* safeLoc = sWorldSafeLocsStore.LookupEntry(m_GraveyardId);
    return player->GetDistance2d(safeLoc->x, safeLoc->y);
}

void BfGraveyard::AddPlayer(uint64 playerGuid)
{
    if (!m_ResurrectQueue.count(playerGuid))
    {
        m_ResurrectQueue.insert(playerGuid);

        if (Player* player = sObjectAccessor->FindPlayer(playerGuid))
            player->CastSpell(player, SPELL_WAITING_FOR_RESURRECT, true);
    }
}

void BfGraveyard::RemovePlayer(uint64 playerGuid)
{
    m_ResurrectQueue.erase(m_ResurrectQueue.find(playerGuid));

    if (Player* player = sObjectAccessor->FindPlayer(playerGuid))
        player->RemoveAurasDueToSpell(SPELL_WAITING_FOR_RESURRECT);
}

void BfGraveyard::Resurrect()
{
    if (m_ResurrectQueue.empty())
        return;

    for (GuidSet::const_iterator itr = m_ResurrectQueue.begin(); itr != m_ResurrectQueue.end(); ++itr)
    {
        Player* player = sObjectAccessor->FindPlayer(*itr);
        if (!player)
            continue;

        if (player->IsInWorld())
            if (Unit* spirit = sObjectAccessor->FindUnit(m_SpiritGuide[m_ControlTeam]))
                spirit->CastSpell(spirit, SPELL_SPIRIT_HEAL, true);

        player->CastSpell(player, SPELL_RESURRECTION_VISUAL, true);
        player->ResurrectPlayer(1.0f);
        player->CastSpell(player, 6962, true);
        player->CastSpell(player, SPELL_SPIRIT_HEAL_MANA, true);

        sObjectAccessor->ConvertCorpseForPlayer(player->GetGUID());
    }

    m_ResurrectQueue.clear();
}

void BfGraveyard::GiveControlTo(TeamId team)
{
    m_ControlTeam = team;
    RelocateDeadPlayers();
}

void BfGraveyard::RelocateDeadPlayers()
{
    WorldSafeLocsEntry const* closestGrave = NULL;
    for (GuidSet::const_iterator itr = m_ResurrectQueue.begin(); itr != m_ResurrectQueue.end(); ++itr)
    {
        Player* player = sObjectAccessor->FindPlayer(*itr);
        if (!player)
            continue;

        if (closestGrave)
            player->TeleportTo(player->GetMapId(), closestGrave->x, closestGrave->y, closestGrave->z, player->GetOrientation());
        else
        {
            closestGrave = m_Bf->GetClosestGraveYard(player);
            if (closestGrave)
                player->TeleportTo(player->GetMapId(), closestGrave->x, closestGrave->y, closestGrave->z, player->GetOrientation());
        }
    }
}

// *******************************************************
// *************** End Spirit Guide system ***************
// *******************************************************
// ********************** Misc ***************************
// *******************************************************

Creature* Battlefield::SpawnCreature(uint32 entry, Position pos, TeamId team)
{
    return SpawnCreature(entry, pos.m_positionX, pos.m_positionY, pos.m_positionZ, pos.m_orientation, team);
}

Creature* Battlefield::SpawnCreature(uint32 entry, float x, float y, float z, float o, TeamId team)
{
    Map* map = const_cast < Map * >(sMapMgr->CreateBaseMap(m_MapId));
    if (!map)
    {
        sLog->outError(LOG_FILTER_BATTLEFIELD, "Battlefield::SpawnCreature: Can't create creature entry: %u map not found", entry);
        return 0;
    }

    Creature* creature = new Creature;
    if (!creature->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_UNIT), map, PHASEMASK_NORMAL, entry, 0, team, x, y, z, o))
    {
        sLog->outError(LOG_FILTER_BATTLEFIELD, "Battlefield::SpawnCreature: Can't create creature entry: %u", entry);
        delete creature;
        return NULL;
    }

    creature->SetHomePosition(x, y, z, o);

    CreatureTemplate const* cinfo = sObjectMgr->GetCreatureTemplate(entry);
    if (!cinfo)
    {
        sLog->outError(LOG_FILTER_BATTLEFIELD, "Battlefield::SpawnCreature: entry %u does not exist.", entry);
        return NULL;
    }

    creature->SetSpeed(MOVE_WALK, cinfo->speed_walk);
    creature->SetSpeed(MOVE_RUN, cinfo->speed_run);
    map->AddToMap(creature);
    creature->setActive(true);

    return creature;
}

GameObject* Battlefield::SpawnGameObject(uint32 entry, uint32 mapid, float x, float y, float z, float o)
{
    Map* map = const_cast<Map*>(sMapMgr->CreateBaseMap(mapid));
    if (!map)
        return 0;

    GameObject* go = new GameObject;
    if (!go->Create(sObjectMgr->GenerateLowGuid(HIGHGUID_GAMEOBJECT), entry, map, PHASEMASK_NORMAL, x, y, z, o, 0, 0, 0, 0, 100, GO_STATE_READY))
    {
        sLog->outError(LOG_FILTER_BATTLEFIELD, "Battlefield::SpawnGameObject: Gameobject template %u not found in database! Battlefield not created!", entry);
        sLog->outError(LOG_FILTER_BATTLEFIELD, "Battlefield::SpawnGameObject: Cannot create gameobject template %u! Battlefield not created!", entry);
        delete go;
        return NULL;
    }

    map->AddToMap(go);
    go->setActive(true);

    return go;
}

// *******************************************************
// ******************* CapturePoint **********************
// *******************************************************

BfCapturePoint::BfCapturePoint(Battlefield* battlefield) : m_Bf(battlefield), m_capturePoint(NULL)
{
    m_team = TEAM_NEUTRAL;
    m_value = 0;
    m_minValue = 0.0f;
    m_maxValue = 0.0f;
    m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_NEUTRAL;
    m_OldState = BF_CAPTUREPOINT_OBJECTIVESTATE_NEUTRAL;
    m_capturePointEntry = 0;
    m_neutralValuePct = 0;
    m_maxSpeed = 0;
}

bool BfCapturePoint::HandlePlayerEnter(Player* player)
{
    if (m_capturePoint)
    {
        player->SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldState1, 1);
        player->SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldstate2, uint32(ceil((m_value + m_maxValue) / (2 * m_maxValue) * 100.0f)));
        player->SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldstate3, m_neutralValuePct);
    }
    return m_activePlayers[player->GetTeamId()].insert(player->GetGUID()).second;
}

GuidSet::iterator BfCapturePoint::HandlePlayerLeave(Player* player)
{
    if (m_capturePoint)
        player->SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldState1, 0);

    GuidSet::iterator current = m_activePlayers[player->GetTeamId()].find(player->GetGUID());

    if (current == m_activePlayers[player->GetTeamId()].end())
        return current;

    m_activePlayers[player->GetTeamId()].erase(current++);
    return current;
}

void BfCapturePoint::SendChangePhase()
{
    if (!m_capturePoint)
        return;

    SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldState1, 1);
    SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldstate2, (uint32) ceil((m_value + m_maxValue) / (2 * m_maxValue) * 100.0f));
    SendUpdateWorldState(m_capturePoint->GetGOInfo()->capturePoint.worldstate3, m_neutralValuePct);
}

bool BfCapturePoint::SetCapturePointData(GameObject* capturePoint)
{
    ASSERT(capturePoint);

    sLog->outDebug(LOG_FILTER_BATTLEFIELD, "Creating capture point %u", capturePoint->GetEntry());

    m_capturePoint = capturePoint;

    GameObjectTemplate const* goinfo = capturePoint->GetGOInfo();
    if (goinfo->type != GAMEOBJECT_TYPE_CAPTURE_POINT)
    {
        sLog->outError(LOG_FILTER_GENERAL, "OutdoorPvP: GO %u is not capture point!", capturePoint->GetEntry());
        return false;
    }

    m_maxValue = goinfo->capturePoint.maxTime;
    m_maxSpeed = m_maxValue / (goinfo->capturePoint.minTime ? goinfo->capturePoint.minTime : 60);
    m_neutralValuePct = goinfo->capturePoint.neutralPercent;
    m_minValue = m_maxValue * goinfo->capturePoint.neutralPercent / 100;
    m_capturePointEntry = capturePoint->GetEntry();
    if (m_team == TEAM_ALLIANCE)
    {
        m_value = m_maxValue;
        m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_ALLIANCE;
    }
    else
    {
        m_value = -m_maxValue;
        m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_HORDE;
    }

    return true;
}

bool BfCapturePoint::DelCapturePoint()
{
    if (m_capturePoint)
    {
        m_capturePoint->SetRespawnTime(0);
        m_capturePoint->Delete();
        m_capturePoint = NULL;
    }

    return true;
}

bool BfCapturePoint::Update(uint32 diff)
{
    if (!m_capturePoint)
        return false;

    float radius = m_capturePoint->GetGOInfo()->capturePoint.radius;

    for (uint8 team = 0; team < 2; ++team)
    {
        for (GuidSet::iterator itr = m_activePlayers[team].begin(); itr != m_activePlayers[team].end();)
        {
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
            {
                if (!m_capturePoint->IsWithinDistInMap(player, radius) || !player->IsOutdoorPvPActive())
                    itr = HandlePlayerLeave(player);
                else
                    ++itr;
            }
            else
                ++itr;
        }
    }

    std::list<Player*> players;
    Trinity::AnyPlayerInObjectRangeCheck checker(m_capturePoint, radius);
    Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(m_capturePoint, players, checker);
    m_capturePoint->VisitNearbyWorldObject(radius, searcher);

    for (std::list<Player*>::iterator itr = players.begin(); itr != players.end(); ++itr)
        if ((*itr)->IsOutdoorPvPActive())
            if (m_activePlayers[(*itr)->GetTeamId()].insert((*itr)->GetGUID()).second)
                HandlePlayerEnter(*itr);

    float fact_diff = ((float) m_activePlayers[0].size() - (float) m_activePlayers[1].size()) * diff / BATTLEFIELD_OBJECTIVE_UPDATE_INTERVAL;
    if (G3D::fuzzyEq(fact_diff, 0.0f))
        return false;

    uint32 Challenger = 0;
    float maxDiff = m_maxSpeed * diff;

    if (fact_diff < 0)
    {
        if (m_State == BF_CAPTUREPOINT_OBJECTIVESTATE_HORDE && m_value <= -m_maxValue)
            return false;

        if (fact_diff < -maxDiff)
            fact_diff = -maxDiff;

        Challenger = HORDE;
    }
    else
    {
        if (m_State == BF_CAPTUREPOINT_OBJECTIVESTATE_ALLIANCE && m_value >= m_maxValue)
            return false;

        if (fact_diff > maxDiff)
            fact_diff = maxDiff;

        Challenger = ALLIANCE;
    }

    float oldValue = m_value;
    TeamId oldTeam = m_team;

    m_OldState = m_State;

    m_value += fact_diff;

    if (m_value < -m_minValue)
    {
        if (m_value < -m_maxValue)
            m_value = -m_maxValue;
        m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_HORDE;
        m_team = TEAM_HORDE;
    }
    else if (m_value > m_minValue)
    {
        if (m_value > m_maxValue)
            m_value = m_maxValue;
        m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_ALLIANCE;
        m_team = TEAM_ALLIANCE;
    }
    else if (oldValue * m_value <= 0)
    {
        if (Challenger == ALLIANCE)
            m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE;
        else if (Challenger == HORDE)
            m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE;
        m_team = TEAM_NEUTRAL;
    }
    else
    {
        if (Challenger == ALLIANCE && (m_OldState == BF_CAPTUREPOINT_OBJECTIVESTATE_HORDE || m_OldState == BF_CAPTUREPOINT_OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE))
            m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE;
        else if (Challenger == HORDE && (m_OldState == BF_CAPTUREPOINT_OBJECTIVESTATE_ALLIANCE || m_OldState == BF_CAPTUREPOINT_OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE))
            m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE;
        m_team = TEAM_NEUTRAL;
    }

    if (G3D::fuzzyNe(m_value, oldValue))
        SendChangePhase();

    if (m_OldState != m_State)
    {
        if (oldTeam != m_team)
            ChangeTeam(oldTeam);
        return true;
    }

    return false;
}

void BfCapturePoint::SendUpdateWorldState(uint32 field, uint32 value)
{
    for (uint8 team = 0; team < 2; ++team)
        for (GuidSet::iterator itr = m_activePlayers[team].begin(); itr != m_activePlayers[team].end(); ++itr)
            if (Player* player = sObjectAccessor->FindPlayer(*itr))
                player->SendUpdateWorldState(field, value);
}

void BfCapturePoint::SendObjectiveComplete(uint32 id, uint64 guid)
{
    uint8 team;
    switch (m_State)
    {
    case BF_CAPTUREPOINT_OBJECTIVESTATE_ALLIANCE:
        team = 0;
        break;
    case BF_CAPTUREPOINT_OBJECTIVESTATE_HORDE:
        team = 1;
        break;
    default:
        return;
    }

    for (GuidSet::iterator itr = m_activePlayers[team].begin(); itr != m_activePlayers[team].end(); ++itr)
        if (Player* player = sObjectAccessor->FindPlayer(*itr))
            player->KilledMonsterCredit(id, guid);
}

bool BfCapturePoint::IsInsideObjective(Player* player) const
{
    return m_activePlayers[player->GetTeamId()].find(player->GetGUID()) != m_activePlayers[player->GetTeamId()].end();
}

bool Battlefield::IncrementQuest(Player *player, uint32 quest, bool complete)
{
    if (!player)
        return false;

    Quest const* pQuest = sObjectMgr->GetQuestTemplate(quest);
    if (!pQuest || player->GetQuestStatus(quest) == QUEST_STATUS_NONE)
        return false;

    if (complete)
    {
        player->CompleteQuest(quest);
        return true;
    }
    else
    {
        for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        {
            int32 creature = pQuest->RequiredNpcOrGo[i];
            if (uint32 spell_id = pQuest->RequiredSpellCast[i])
            {
                player->CastedCreatureOrGO(creature, 0, spell_id);
                return true;
            }
            else if (creature > 0)
            {
                player->KilledMonsterCredit(creature, 0);
                return true;
            }
            else if (creature < 0)
            {
                player->CastedCreatureOrGO(creature, 0, 0);
                return true;
            }
            return true;
        }
    }
    return false;
}

void BfGraveyard::Initialize(uint32 horde_entry, uint32 alliance_entry, float x, float y, float z, float o, TeamId startcontrol, uint32 gy)
{
    m_ControlTeam = startcontrol;

    if (Creature* cre = m_Bf->SpawnCreature(horde_entry,x,y,z,o,TEAM_HORDE))
    {
        m_SpiritGuide[TEAM_HORDE] = cre->GetGUID();
        cre->SetReactState(REACT_PASSIVE);
        if (m_ControlTeam==TEAM_ALLIANCE)
        {
            cre->SetVisible(false);
        }
    }
    else
        sLog->outError(LOG_FILTER_BATTLEFIELD, "BfGraveYard::Init can't spawn horde spiritguide %u", horde_entry);

    if (Creature* cre = m_Bf->SpawnCreature(alliance_entry, x, y, z, o, TEAM_ALLIANCE))
    {
        m_SpiritGuide[TEAM_ALLIANCE] = cre->GetGUID();
        cre->SetReactState(REACT_PASSIVE);
        if (m_ControlTeam==TEAM_HORDE)
        {
            cre->SetVisible(false);
        }
    }
    else
        sLog->outError(LOG_FILTER_BATTLEFIELD, "BfGraveYard::Init can't spawn alliance spiritguide %u", alliance_entry);

    m_GraveyardId = gy;
}

bool BfCapturePoint::SetCapturePointData(uint32 entry, uint32 map, float x, float y, float z, float o)
{
    sLog->outDebug(LOG_FILTER_BATTLEGROUND, "Creating capture point %u", entry);

    GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(entry);
    if (!goinfo || goinfo->type != GAMEOBJECT_TYPE_CAPTURE_POINT)
    {
        sLog->outError(LOG_FILTER_OUTDOORPVP, "OutdoorPvP: GO %u is not capture point!", entry);
        return false;
    }

    if (m_capturePoint = m_Bf->SpawnGameObject(entry,map,x,y,z,o))
    {    
        m_maxValue = goinfo->capturePoint.maxTime;
        m_maxSpeed = m_maxValue / (goinfo->capturePoint.minTime ? goinfo->capturePoint.minTime : 60);
        m_neutralValuePct = goinfo->capturePoint.neutralPercent;
        m_minValue = m_maxValue * goinfo->capturePoint.neutralPercent / 100;
        m_capturePointEntry = entry;
        if (m_team == TEAM_ALLIANCE)
        {
            m_value = m_maxValue;
            m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_ALLIANCE;
        }
        else 
        {
            m_value = -m_maxValue;
            m_State = BF_CAPTUREPOINT_OBJECTIVESTATE_HORDE;
        }
        return true;
    }
    else
        return false;
}
