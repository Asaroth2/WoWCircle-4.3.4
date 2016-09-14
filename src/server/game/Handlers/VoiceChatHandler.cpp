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


#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "ChannelMgr.h"
#include "Channel.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include "SocialMgr.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "AccountMgr.h"

void WorldSession::HandleVoiceSessionEnableOpcode(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_VOICE_SESSION_ENABLE");
	
	bool VoiceEnabled, MicrophoneEnabled;

    recvData >> VoiceEnabled;
    recvData >> MicrophoneEnabled;
    // Something lacking here.
}

void WorldSession::HandleChannelVoiceOnOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_CHANNEL_VOICE_ON");
    
    uint32 length = recvPacket.ReadBits(8);
    std::string channelname = recvPacket.ReadString(length);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->MakeVoiceOn(&recvPacket, _player->GetGUID());
}

void WorldSession::HandleChannelVoiceOffOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_CHANNEL_VOICE_OFF");
    
    uint32 length = recvPacket.ReadBits(8);
    std::string channelname = recvPacket.ReadString(length);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->MakeVoiceOff(&recvPacket, _player->GetGUID());
}

void WorldSession::HandleSetActiveVoiceChannel(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_SET_ACTIVE_VOICE_CHANNEL");

    uint32 channelId;
    std::string channelname;

    recvPacket >> channelId;
    uint32 length = recvPacket.ReadBits(8);
    channelname = recvPacket.ReadString(length);
}