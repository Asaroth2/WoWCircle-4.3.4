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


#include "AccountMgr.h"
#include "DatabaseEnv.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Util.h"
#include "SHA1.h"

namespace AccountMgr
{

AccountOpResult CreateAccount(std::string username, std::string password)
{
    if (utf8length(username) > MAX_ACCOUNT_STR)
        return AOR_NAME_TOO_LONG;                           // username's too long

    normalizeString(username);
    normalizeString(password);

    if (GetId(username))
        return AOR_NAME_ALREDY_EXIST;                       // username does already exist

    LoginDatabase.PExecute("INSERT INTO account(username, sha_pass_hash, joindate) VALUES('%s', '%s', NOW())", username.c_str(), CalculateShaPassHash(username, password).c_str());

    return AOR_OK;                                          // everything's fine
}

AccountOpResult DeleteAccount(uint32 accountId)
{
    // Check if accounts exists
    QueryResult result = LoginDatabase.PQuery("SELECT 1 FROM account WHERE id='%d'", accountId);
    if (!result)
        return AOR_NAME_NOT_EXIST;

    // Obtain accounts characters
    result = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE account='%d'", accountId);
    if (result)
    {
        do
        {
            uint32 guidLow = (*result)[0].GetUInt32();
            uint64 guid = MAKE_NEW_GUID(guidLow, 0, HIGHGUID_PLAYER);

            // Kick if player is online
            if (Player* p = ObjectAccessor::FindPlayer(guid))
            {
                WorldSession* s = p->GetSession();
                s->KickPlayer();                            // mark session to remove at next session list update
                s->LogoutPlayer(false);                     // logout player without waiting next session list update
            }

            Player::DeleteFromDB(guid, accountId, false);       // no need to update realm characters
        } while (result->NextRow());
    }

    // table realm specific but common for all characters of account for realm
    CharacterDatabase.PExecute("DELETE FROM character_tutorial WHERE account = '%u'", accountId);
    CharacterDatabase.PExecute("DELETE FROM account_data WHERE account = '%u'", accountId);
    CharacterDatabase.PExecute("DELETE cb FROM character_banned cb INNER JOIN characters c ON c.guid = cb.guid WHERE c.account = '%u'", accountId);

    SQLTransaction trans = LoginDatabase.BeginTransaction();

    trans->PAppend("DELETE FROM account WHERE id='%d'", accountId);
    trans->PAppend("DELETE FROM account_access WHERE id ='%d'", accountId);
    trans->PAppend("DELETE FROM realmcharacters WHERE acctid='%d'", accountId);

    LoginDatabase.CommitTransaction(trans);

    return AOR_OK;
}

AccountOpResult ChangeUsername(uint32 accountId, std::string newUsername, std::string newPassword)
{
    // Check if accounts exists
    QueryResult result = LoginDatabase.PQuery("SELECT 1 FROM account WHERE id='%d'", accountId);
    if (!result)
        return AOR_NAME_NOT_EXIST;

    if (utf8length(newUsername) > MAX_ACCOUNT_STR)
        return AOR_NAME_TOO_LONG;

    if (utf8length(newPassword) > MAX_ACCOUNT_STR)
        return AOR_PASS_TOO_LONG;

    normalizeString(newUsername);
    normalizeString(newPassword);

    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_USERNAME);

    stmt->setString(0, newUsername);
    stmt->setString(1, CalculateShaPassHash(newUsername, newPassword));
    stmt->setUInt32(2, accountId);

    LoginDatabase.Execute(stmt);

    return AOR_OK;
}

AccountOpResult ChangePassword(uint32 accountId, std::string newPassword)
{
    std::string username;

    if (!GetName(accountId, username))
        return AOR_NAME_NOT_EXIST;                          // account doesn't exist

    if (utf8length(newPassword) > MAX_ACCOUNT_STR)
        return AOR_PASS_TOO_LONG;

    normalizeString(username);
    normalizeString(newPassword);

    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_PASSWORD);

    stmt->setString(0, CalculateShaPassHash(username, newPassword));
    stmt->setUInt32(1, accountId);

    LoginDatabase.Execute(stmt);

    return AOR_OK;
}

uint32 GetId(std::string username)
{
    LoginDatabase.EscapeString(username);
    QueryResult result = LoginDatabase.PQuery("SELECT id FROM account WHERE username = '%s'", username.c_str());
    if (!result)
        return 0;
    else
    {
        uint32 id = (*result)[0].GetUInt32();
        return id;
    }
}

uint32 GetSecurity(uint32 accountId)
{
    QueryResult result = LoginDatabase.PQuery("SELECT gmlevel FROM account_access WHERE id = '%u'", accountId);
    if (result)
    {
        uint32 sec = (*result)[0].GetUInt32();
        return sec;
    }

    return 0;
}

uint32 GetSecurity(uint32 accountId, int32 realmId)
{
    QueryResult result = (realmId == -1)
        ? LoginDatabase.PQuery("SELECT gmlevel FROM account_access WHERE id = '%u' AND RealmID = '%d'", accountId, realmId)
        : LoginDatabase.PQuery("SELECT gmlevel FROM account_access WHERE id = '%u' AND (RealmID = '%d' OR RealmID = '-1')", accountId, realmId);
    if (result)
    {
        uint32 sec = (*result)[0].GetUInt32();
        return sec;
    }

    return 0;
}

bool GetName(uint32 accountId, std::string& name)
{
    QueryResult result = LoginDatabase.PQuery("SELECT username FROM account WHERE id = '%u'", accountId);
    if (result)
    {
        name = (*result)[0].GetString();
        return true;
    }

    return false;
}

bool CheckPassword(uint32 accountId, std::string password)
{
    std::string username;

    if (!GetName(accountId, username))
        return false;

    normalizeString(username);
    normalizeString(password);

    QueryResult result = LoginDatabase.PQuery("SELECT 1 FROM account WHERE id='%d' AND sha_pass_hash='%s'", accountId, CalculateShaPassHash(username, password).c_str());
    return (result) ? true : false;
}

bool IsEmptyPassword(uint32 accountId)
{
    QueryResult result = LoginDatabase.PQuery("SELECT 1 FROM account WHERE id ='%d' AND sha_pass_hash = ''", accountId);
    return result ? true : false;
}

uint32 GetCharactersCount(uint32 accountId)
{
    uint32 charcount = 0;
    // check character count
    QueryResult result = CharacterDatabase.PQuery("SELECT COUNT(guid) FROM characters WHERE account = '%d'", accountId);
    if (result)
    {
        Field *fields=result->Fetch();
        charcount = fields[0].GetUInt32();
    }
    return charcount;
}

bool normalizeString(std::string& utf8String)
{
    wchar_t buffer[MAX_ACCOUNT_STR+1];

    size_t maxLength = MAX_ACCOUNT_STR;
    if (!Utf8toWStr(utf8String, buffer, maxLength))
        return false;
#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif
    std::transform(&buffer[0], buffer+maxLength, &buffer[0], wcharToUpperOnlyLatin);
#ifdef _MSC_VER
#pragma warning(default: 4996)
#endif

    return WStrToUtf8(buffer, maxLength, utf8String);
}

std::string CalculateShaPassHash(std::string& name, std::string& password)
{
    SHA1Hash sha;
    sha.Initialize();
    sha.UpdateData(name);
    sha.UpdateData(":");
    sha.UpdateData(password);
    sha.Finalize();

    return ByteArrayToHexStr(sha.GetDigest(), sha.GetLength());
}

bool IsPlayerAccount(uint32 gmlevel)
{
    return gmlevel == SEC_PLAYER;
}

bool IsModeratorAccount(uint32 gmlevel)
{
    return gmlevel > SEC_MODERATOR && gmlevel <= SEC_CONSOLE;
}

bool IsGMAccount(uint32 gmlevel)
{
    return gmlevel >= SEC_GAMEMASTER && gmlevel <= SEC_CONSOLE;
}

bool IsAdminAccount(uint32 gmlevel)
{
    return gmlevel >= SEC_ADMINISTRATOR && gmlevel <= SEC_CONSOLE;
}

bool IsConsoleAccount(uint32 gmlevel)
{
    return gmlevel == SEC_CONSOLE;
}

} // Namespace AccountMgr