/* hack script for dk base camp teleport */
#include "ScriptPCH.h"
#include "ScriptedEscortAI.h"
#include "Spell.h"
#include "ObjectMgr.h"


// Options
enum eEnums
{
    CHANGE_ITEMBACK             = 0,
    EFIRALS                     = 38186,
    CHANGE_FACTION              = 1,
    CHANGE_RACE                 = 2,
    CHANGE_GENDER               = 3,
    RECOVERY_CHAR               = 4,
    EFIRALS_TRANS               = 5,

    CHANGE_FACTION_COUNT        = 200,
    CHANGE_RACE_COUNT           = 150,
    CHANGE_GENDER_COUNT         = 100,
    RECOVERY_CHAR_COUNT         = 225,
};

class multi_vendor : public CreatureScript
{
public:
    multi_vendor() : CreatureScript("multi_vendor"){}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        float rate = sWorld->getRate(RATE_DONATE);
        uint32 priceFaction = uint32(200 * rate);
        uint32 priceRace = uint32(150 * rate);
        uint32 priceGender = uint32(100 * rate);
        uint32 priceRecobery = uint32(225 * rate);

        LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
        char printinfo[500]; // Max length

        sprintf(printinfo, sObjectMgr->GetTrinityString(20003, loc_idx), priceFaction);
        player->ADD_GOSSIP_ITEM(0, printinfo, GOSSIP_SENDER_MAIN, CHANGE_FACTION);

        sprintf(printinfo, sObjectMgr->GetTrinityString(20004, loc_idx), priceRace);
        player->ADD_GOSSIP_ITEM(0, printinfo, GOSSIP_SENDER_MAIN, CHANGE_RACE);

        sprintf(printinfo, sObjectMgr->GetTrinityString(20005, loc_idx), priceGender);
        player->ADD_GOSSIP_ITEM(0, printinfo, GOSSIP_SENDER_MAIN, CHANGE_GENDER);

        sprintf(printinfo, sObjectMgr->GetTrinityString(20010, loc_idx), priceRecobery);
        //player->ADD_GOSSIP_ITEM(0, printinfo, GOSSIP_SENDER_MAIN, RECOVERY_CHAR);
        player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20014, loc_idx), GOSSIP_SENDER_MAIN, EFIRALS_TRANS);

        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
    {
        if(!player || !creature || !player->getAttackers().empty())
            return true;

        float rate = sWorld->getRate(RATE_DONATE);
        uint32 priceFaction = uint32(200 * rate);
        uint32 priceRace = uint32(150 * rate);
        uint32 priceGender = uint32(100 * rate);
        uint32 priceRecobery = uint32(225 * rate);
        uint32 countefirs = player->GetItemCount(EFIRALS, false);
        uint32 accountId = player->GetSession()->GetAccountId();
        LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
        if(accountId < 1)
            return true;

        bool activate = false;
        ChatHandler chH = ChatHandler(player);

        switch(sender)
        {
            case GOSSIP_SENDER_MAIN:
            {
                switch(action)
                {
                    case CHANGE_FACTION:
                    {
                        if(countefirs < priceFaction)
                        {
                            chH.PSendSysMessage(20000, priceFaction);
                            player->CLOSE_GOSSIP_MENU();
                            break;
                        }

                        player->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
                        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '64' WHERE guid = %u", player->GetGUIDLow());
                        CharacterDatabase.PExecute("INSERT INTO character_donate_service SET `account`='%u',`guid`='%u', `service`='%s', `cost`='%u', `targetguid`='%u'",accountId , player->GetGUIDLow(), "CHANGE_FACTION", priceFaction, player->GetGUIDLow());
                        player->DestroyItemCount(EFIRALS, priceFaction, true);
                        activate = true;
                        player->CLOSE_GOSSIP_MENU();
                        break;
                    }
                    case CHANGE_RACE:
                    {
                        if(countefirs < priceRace)
                        {
                            chH.PSendSysMessage(20000, priceRace);
                            player->CLOSE_GOSSIP_MENU();
                            break;
                        }

                        player->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
                        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '128' WHERE guid = %u", player->GetGUIDLow());
                        CharacterDatabase.PExecute("INSERT INTO character_donate_service SET `account`='%u',`guid`='%u', `service`='%s', `cost`='%u', `targetguid`='%u'", accountId, player->GetGUIDLow(), "CHANGE_RACE", priceRace, player->GetGUIDLow());
                        player->DestroyItemCount(EFIRALS, priceRace, true);
                        activate = true;
                        player->CLOSE_GOSSIP_MENU();
                        break;
                    }
                    case CHANGE_GENDER:
                    {
                        if(countefirs < priceGender)
                        {
                            chH.PSendSysMessage(20000, priceGender);
                            break;
                        }

                        player->SetAtLoginFlag(AT_LOGIN_CUSTOMIZE);
                        CharacterDatabase.PExecute("UPDATE characters SET at_login = at_login | '8' WHERE guid = '%u'", player->GetGUIDLow());
                        CharacterDatabase.PExecute("INSERT INTO character_donate_service SET `account`='%u',`guid`='%u', `service`='%s', `cost`='%u', `targetguid`='%u'", accountId, player->GetGUIDLow(), "CHANGE_GENDER", priceGender, player->GetGUIDLow());
                        player->DestroyItemCount(EFIRALS, priceGender, true);
                        activate = true;
                        player->CLOSE_GOSSIP_MENU();
                        break;
                    }
                    case EFIRALS_TRANS:
                    {
                        if(countefirs < 10 || countefirs > 9999)
                        {
                            chH.PSendSysMessage(20018);
                            player->CLOSE_GOSSIP_MENU();
                            break;
                        }

                        QueryResult result = CharacterDatabase.PQuery("SELECT name, guid FROM `characters` WHERE account = '%u' AND guid != '%u'", accountId, player->GetGUIDLow());
                        if (!result)
                        {
                            player->CLOSE_GOSSIP_MENU();
                            player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20015, loc_idx), GOSSIP_SENDER_MAIN, CHANGE_ITEMBACK);
                            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
                        }
                        else
                        {
                            player->CLOSE_GOSSIP_MENU();
                            uint32 transcount = CalculatePct(countefirs, 85);
                            uint32 delcount = (countefirs - transcount);
                            char printinfo[500]; // Max length
                            sprintf(printinfo, sObjectMgr->GetTrinityString(20016, loc_idx), countefirs, transcount, delcount);
                            player->ADD_GOSSIP_ITEM(0, printinfo, GOSSIP_SENDER_MAIN, EFIRALS_TRANS);
                            do
                            {
                                Field* fields = result->Fetch();
                                std::string playerName  = fields[0].GetString();
                                uint32 guid             = fields[1].GetUInt32();

                                char chardata[50]; // Max length: name(12) + guid(11) + _.log (5) + \0
                                sprintf(chardata, "%s", playerName.c_str());
                                player->ADD_GOSSIP_ITEM(0, chardata, GOSSIP_SENDER_INN_INFO, guid);

                            }while (result->NextRow());
                        }
                        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
                        return true;
                    }
                    case RECOVERY_CHAR:
                    {
                        QueryResult result = CharacterDatabase.PQuery("SELECT deleteInfos_Name, guid, level, class, deleteDate FROM `characters` WHERE deleteInfos_Account = '%u'", accountId);
                        if (!result)
                        {
                            player->CLOSE_GOSSIP_MENU();
                            player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20009, loc_idx), GOSSIP_SENDER_MAIN, CHANGE_ITEMBACK);
                            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
                        }
                        else
                        {
                            player->CLOSE_GOSSIP_MENU();
                            do
                            {
                                Field* fields = result->Fetch();
                                std::string playerName  = fields[0].GetString();
                                uint32 guid             = fields[1].GetUInt32();
                                uint8 level             = fields[2].GetUInt8();
                                uint8 pclass            = fields[3].GetUInt8();
                                uint32 deleteDate       = fields[4].GetUInt32();

                                char chardata[200]; // Max length: name(12) + guid(11) + _.log (5) + \0
                                sprintf(chardata, "%s level %d class %s", playerName.c_str(), level, GetClassNameById(pclass).c_str());
                                player->ADD_GOSSIP_ITEM(0, chardata, GOSSIP_SENDER_INFO, guid);

                            }while (result->NextRow());
                        }

                        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
                        return true;
                    }
                }
                break;
            }
            case GOSSIP_SENDER_INN_INFO:
            {
                if(countefirs < 10 || countefirs > 9999)
                {
                    chH.PSendSysMessage(20018);
                    player->CLOSE_GOSSIP_MENU();
                    break;
                }
                QueryResult result = CharacterDatabase.PQuery("SELECT name, guid FROM `characters` WHERE account = '%u' AND guid = '%u'", accountId, action);
                if (!result || action == player->GetGUIDLow())
                {
                    player->CLOSE_GOSSIP_MENU();
                    break;
                }
                else
                {
                    Player* receiver = ObjectAccessor::FindPlayer(action);
                    uint32 transcount = CalculatePct(countefirs, 85);
                    player->DestroyItemCount(EFIRALS, countefirs, true);
                    // from console show not existed sendermail
                    MailSender sendermail(MAIL_NORMAL, 0, MAIL_STATIONERY_GM);
                    // fill mail
                    MailDraft draft(sObjectMgr->GetTrinityString(20017, loc_idx), sObjectMgr->GetTrinityString(20017, loc_idx));
                    SQLTransaction trans = CharacterDatabase.BeginTransaction();

                    if (Item* item = Item::CreateItem(EFIRALS, transcount, 0))
                    {
                        player->SaveInventoryAndGoldToDB(trans);
                        item->SaveToDB(trans);                           // save for prevent lost at next mail load, if send fail then item will deleted
                        draft.AddItem(item);
                        draft.SendMailTo(trans, MailReceiver(receiver, GUID_LOPART(action)), sendermail);
                        CharacterDatabase.CommitTransaction(trans);
                        chH.PSendSysMessage(20019, transcount);
                    }
                    CharacterDatabase.PExecute("INSERT INTO character_donate_service SET `account`='%u',`guid`='%u', `service`='%s', `cost`='%u', `targetguid`='%u'",accountId , player->GetGUIDLow(), "EFIRALS_TRANS", transcount, action);
                }
                break;
            }
            case GOSSIP_SENDER_INFO:
            {
                if(countefirs < priceRecobery)
                {
                    chH.PSendSysMessage(20000, priceRecobery);
                    player->CLOSE_GOSSIP_MENU();
                    break;
                }
                std::string name = "";
                uint8 gender = GENDER_NONE;
                uint8 race = RACE_NONE;
                uint8 playerClass = 0;
                uint8 level = 1;
                bool setrename = false;

                QueryResult result = CharacterDatabase.PQuery("SELECT deleteInfos_Account, guid, deleteInfos_Name, gender, race, class, level FROM `characters` WHERE guid = '%u'", action);
                if (result)
                {
                    Field* fields = result->Fetch();
                    uint32 account = fields[0].GetUInt32();
                    name = fields[2].GetString();
                    gender = fields[3].GetUInt8();
                    race = fields[4].GetUInt8();
                    playerClass = fields[5].GetUInt8();
                    level = fields[6].GetUInt8();
                    if(!account || account != accountId)
                    {
                        player->CLOSE_GOSSIP_MENU();
                        break;
                    }
                    else
                    {
                        QueryResult result2 = CharacterDatabase.PQuery("SELECT name FROM characters WHERE name = '%s'", name.c_str());
                        if(result2)
                            setrename = true;
                    }
                }
                else
                {
                    player->CLOSE_GOSSIP_MENU();
                    break;
                }

                if(setrename)
                    CharacterDatabase.PExecute("UPDATE `characters` SET at_login = at_login | '1' WHERE AND guid = '%u'", action);

                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                trans->PAppend("UPDATE `characters` SET name = deleteInfos_Name, account = deleteInfos_Account, deleteDate = NULL, deleteInfos_Name = NULL, deleteInfos_Account = NULL WHERE deleteDate IS NOT NULL AND guid = '%u'", action);
                CharacterDatabase.CommitTransaction(trans);

                player->DestroyItemCount(EFIRALS, priceRecobery, true);
                sWorld->AddCharacterNameData(action, name, gender, race, playerClass, level);
                CharacterDatabase.PExecute("INSERT INTO character_donate_service SET `account`='%u',`guid`='%u', `service`='%s', `cost`='%u', `targetguid`='%u'",accountId , player->GetGUIDLow(), "RECOVERY_CHAR", priceRecobery, action);
                chH.PSendSysMessage(20011);
            }
            break;
        }

        if(activate)
            chH.PSendSysMessage(LANG_CUSTOMIZE_PLAYER, chH.GetNameLink(player).c_str());

        player->CLOSE_GOSSIP_MENU();
        return true;
    }
    std::string GetClassNameById(uint8 id)
    {
        std::string sClass = "";
        switch (id)
        {
            case CLASS_WARRIOR:         sClass = "Warrior";        break;
            case CLASS_PALADIN:         sClass = "Pala";           break;
            case CLASS_HUNTER:          sClass = "Hunt";           break;
            case CLASS_ROGUE:           sClass = "Rogue";          break;
            case CLASS_PRIEST:          sClass = "Priest";         break;
            case CLASS_DEATH_KNIGHT:    sClass = "DK";             break;
            case CLASS_SHAMAN:          sClass = "Shama";          break;
            case CLASS_MAGE:            sClass = "Mage";           break;
            case CLASS_WARLOCK:         sClass = "Warlock";        break;
            case CLASS_DRUID:           sClass = "Druid";          break;
        }
        return sClass;
    }
};

class item_back : public CreatureScript
{
public:
    item_back() : CreatureScript("item_back"){}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        QueryResult result = CharacterDatabase.PQuery("SELECT itemguid, itemEntry, count FROM character_donate WHERE owner_guid = '%u' AND state = 0", player->GetGUIDLow());
        if (!result)
        {
            LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
            player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20008, loc_idx), GOSSIP_SENDER_MAIN, CHANGE_ITEMBACK);
            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
        }
        else
        {
            LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
            player->ADD_GOSSIP_ITEM(0, sObjectMgr->GetTrinityString(20006, loc_idx), GOSSIP_SENDER_MAIN, CHANGE_ITEMBACK);
            do
            {
                Field* fields = result->Fetch();
                uint32 item_guid = fields[0].GetUInt32();
                uint32 entry = fields[1].GetUInt32();
                uint32 count = fields[2].GetUInt32();
                ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(entry);
                if (pProto && pProto->Stackable < 2)
                {
                    std::string Name = pProto->Name1;

                    if (loc_idx >= 0)
                    {
                        if (ItemLocale const* il = sObjectMgr->GetItemLocale(pProto->ItemId))
                            ObjectMgr::GetLocaleString(il->Name, loc_idx, Name);
                    }

                    player->ADD_GOSSIP_ITEM(0, Name, GOSSIP_SENDER_MAIN, item_guid);
                }
            }while (result->NextRow());
        }

        player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE,creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
    {
        if(!player || !creature || sender != GOSSIP_SENDER_MAIN || !player->getAttackers().empty())
            return true;

        ChatHandler chH = ChatHandler(player);

        if(action > 0)
        {
            QueryResult result = CharacterDatabase.PQuery("SELECT `owner_guid`, `itemEntry`, `efircount`, `count` FROM character_donate WHERE itemguid = '%u' AND state = 0", action);
            if(!result)
            {
                player->CLOSE_GOSSIP_MENU(); return true;
            }

            Field* fields = result->Fetch();
            uint32 owner_guid = fields[0].GetUInt32();
            uint32 entry = fields[1].GetUInt32();
            uint32 efircount = fields[2].GetUInt32();
            uint32 count = fields[3].GetUInt32();
            uint32 countitem = player->GetItemCount(entry, false);

            if(owner_guid != player->GetGUIDLow())
            {
                player->CLOSE_GOSSIP_MENU(); return true;
            }

            QueryResult result2 = CharacterDatabase.PQuery("SELECT `owner_guid` FROM item_instance WHERE guid = '%u'", action);
            if(!result2)
            {
                if(countitem == 0)
                    CharacterDatabase.PExecute("UPDATE character_donate SET state = 3 WHERE itemguid = '%u'", action);

                player->CLOSE_GOSSIP_MENU(); return true;
            }

            Field* fields2 = result2->Fetch();
            uint32 owner_guid2 = fields2[0].GetUInt32();
            if(owner_guid != owner_guid2)
            {
                player->CLOSE_GOSSIP_MENU(); return true;
            }

            if(player->GetGUIDLow() != owner_guid2)
            {
                player->CLOSE_GOSSIP_MENU(); return true;
            }

            if(Item *item = GetItemByGuid(action, player))
            {
                player->DestroyItemCount(item, count, true);
                player->AddItem(EFIRALS, uint32(efircount * 0.8));
                chH.PSendSysMessage(20002, uint32(efircount * 0.8));
            }
        }
        else
            chH.PSendSysMessage(20001);

        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    Item* GetItemByGuid(uint64 guid, Player* player) const
    {
        for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
            if (Item *pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if (pItem->GetGUIDLow() == guid)
                    return pItem;

        for (int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
            if (Item *pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                if (pItem->GetGUIDLow() == guid)
                    return pItem;

        for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
            if (Bag *pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        if (pItem->GetGUIDLow() == guid)
                            return pItem;

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
            if (Bag *pBag = player->GetBagByPos(i))
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                    if (Item* pItem = pBag->GetItemByPos(j))
                        if (pItem->GetGUIDLow() == guid)
                            return pItem;

        return NULL;
    }
};

void AddSC_multi_vendor()
{
    new multi_vendor();
    new item_back();
}