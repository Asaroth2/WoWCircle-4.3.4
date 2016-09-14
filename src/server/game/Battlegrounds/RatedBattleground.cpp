/*
 * Copyright (C)   2013    uWoW <http://uwow.biz>
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

#include "RatedBattleground.h"
#include "DatabaseEnv.h"


uint16 RatedBattleground::ConquestPointReward = 400;

float GetChanceAgainst(int ownRating, int opponentRating)
{
    // Returns the chance to win against a team with the given rating, used in the rating adjustment calculation
    // ELO system
    return 1.0f / (1.0f + exp(log(10.0f) * (float)((float)opponentRating - (float)ownRating) / 400.0f));
}

int GetRatingMod(int ownRating, int opponentRating, bool won /*, float confidence_factor*/)
{
    // 'Chance' calculation - to beat the opponent
    // This is a simulation. Not much info on how it really works
    float chance = GetChanceAgainst(ownRating, opponentRating);
    float won_mod = (won) ? 1.0f : 0.0f;

    // Calculate the rating modification
    float mod;

    // TODO: Replace this hack with using the confidence factor (limiting the factor to 2.0f)
    if (won && ownRating < 1300)
    {
        if (ownRating < 1000)
            mod = 48.0f * (won_mod - chance);
        else
            mod = (24.0f + (24.0f * (1300.0f - float(ownRating)) / 300.0f)) * (won_mod - chance);
    }
    else
        mod = 24.0f * (won_mod - chance);

    return (int)ceil(mod);

}


int GetMatchmakerRatingMod(int ownRating, int opponentRating, bool won )
{
    // 'Chance' calculation - to beat the opponent
    // This is a simulation. Not much info on how it really works
    float chance = GetChanceAgainst(ownRating, opponentRating);
    float won_mod = (won) ? 1.0f : 0.0f;
    float mod = won_mod - chance;

    // Work in progress:
    /*
    // This is a simulation, as there is not much info on how it really works
    float confidence_mod = min(1.0f - fabs(mod), 0.5f);

    // Apply confidence factor to the mod:
    mod *= confidence_factor

    // And only after that update the new confidence factor
    confidence_factor -= ((confidence_factor - 1.0f) * confidence_mod) / confidence_factor;
    */

    // Real rating modification
    mod *= 24.0f;

    return (int)ceil(mod);
}

RatedBattleground::RatedBattleground(uint64 pGuid) :
    m_owner(pGuid)
{
    m_gamesStats.week_games = 0;
    m_gamesStats.week_wins  = 0;
    m_gamesStats.games      = 0;
    m_gamesStats.wins       = 0;

    m_rating = 0;
    m_mmv    = 1500;
}

RatedBattleground::~RatedBattleground()
{
}

uint32 RatedBattleground::getGames()
{
    return m_gamesStats.games;
}

uint32 RatedBattleground::getWins()
{
    return m_gamesStats.wins;
}

uint32 RatedBattleground::getWeekGames()
{
    return m_gamesStats.week_games;
}

uint32 RatedBattleground::getWeekWins()
{
    return m_gamesStats.week_wins;
}

void RatedBattleground::LoadStats()
{
    if (QueryResult result = CharacterDatabase.PQuery("SELECT `rating`, `mmv`, `games`, `wins`, `weekGames`, `weekWins` FROM character_rated_bg WHERE id = %llu", m_owner))
    {
        Field* fields = result->Fetch();

        m_rating = fields[0].GetUInt16();
        m_mmv = fields[1].GetUInt16();

        m_gamesStats.games      = fields[2].GetUInt32();
        m_gamesStats.wins       = fields[3].GetUInt32();
        m_gamesStats.week_games = fields[4].GetUInt16();
        m_gamesStats.week_wins  = fields[5].GetUInt16();
    }
}

void RatedBattleground::SaveStats()
{
    CharacterDatabase.PExecute("REPLACE INTO `character_rated_bg` VALUES(%llu, %u, %u, %u, %u, %u, %u)",
                               m_owner, m_rating, m_mmv, m_gamesStats.games, m_gamesStats.wins, m_gamesStats.week_games, m_gamesStats.week_wins);
}

uint16 RatedBattleground::FinishGame(bool win, uint16 opponents_mmv)
{
    m_gamesStats.games++;
    m_gamesStats.week_games++;

    if (win)
    {
        m_gamesStats.wins++;
        m_gamesStats.week_wins++;
    }

    int16 mod = (win) ? WonAgainst(opponents_mmv) : LostAgainst(opponents_mmv);

    m_rating += mod;
    m_mmv += GetMatchmakerRatingMod(m_mmv, opponents_mmv, win);

    SaveStats();

    return mod;
}

int16 RatedBattleground::WonAgainst(uint16 opponents_mmv)
{
    return GetRatingMod(m_rating, opponents_mmv, true);
}

int16 RatedBattleground::LostAgainst(uint16 opponents_mmv)
{
    return GetRatingMod(m_rating, opponents_mmv, false);
}
