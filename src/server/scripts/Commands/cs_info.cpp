/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2009-2012 Eilo <https://github.com/eilo>
 * Copyright (C) 2010-2012 WowRean <http://www.wowrean.es/>
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

/* ScriptData
Name: info_commandscript
%Complete: 100
Comment: Info util commands, displaying gs, ilvl talent spec and possible rol
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "GroupMgr.h"
#include "AccountMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Spell.h"
#include "Group.h"
#include "Map.h"
#include "Language.h"
#include "Player.h"
#include "MapManager.h"
#include "MapInstanced.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "LFG.h"
#include "LFGMgr.h"

class info_commandscript : public CommandScript
{
public:
    info_commandscript() : CommandScript("info_commandscript") { }

    ChatCommand* GetCommands() const
    {
        static ChatCommand raidInfoCommandTable[] =
        {
            { "info",           RBAC_PERM_COMMAND_RAID_INFO,         false, &HandleRaidInfoCommand,            "", NULL },
            { "list",           RBAC_PERM_COMMAND_RAID_LIST,         false, &HandleRaidListCommand,            "", NULL },
            { NULL,             0,                  false, NULL,                              "", NULL }
        };
        static ChatCommand playerInfoCommandTable[] =
        {
            { "info",           RBAC_PERM_COMMAND_PLAYER_INFO,         false, &HandlePlayerInfoCommand,          "", NULL },
            { NULL,             0,                  false, NULL,                              "", NULL }
        };
        static ChatCommand commandTable[] =
        {
            { "player",         RBAC_PERM_COMMAND_PLAYER_INFO,         true,  NULL,            "", playerInfoCommandTable },
            { "raid",           RBAC_PERM_COMMAND_RAID_INFO,         true,  NULL,              "", raidInfoCommandTable },
            { NULL,             0,                  false, NULL,                              "", NULL }
        };
        return commandTable;
    }

    static bool HandlePlayerInfoCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        uint64 targetGuid;
        std::string name("Ninguno");
        if (!handler->extractPlayerTarget((char*)args, &target, &targetGuid, &name))
            return false;

        if (!target)
            return false;

        if (!target->IsInWorld())
            return false;

        std::string guild("Ninguna");
        if (target->GetGuildId() != 0)
           guild = target->GetGuildName();

        std::string clase("Ninguna");
        std::string spec("Ninguna");
        std::string rol("Ninguno");

        uint8 spec1 = 0;
        uint8 spec2 = 0;
        uint8 spec3 = 0;

        uint32 gs = 0;
        uint32 ilvl = 0;

        uint16 mmrdos = 0;
        uint16 mmrtres = 0;
        uint16 mmrcinco = 0;

        // Handling of talent count
        uint32 const* talentTabIds = GetTalentTabPages(target->getClass());

        for (uint8 i = 0; i < MAX_TALENT_TABS; ++i)
        {
            uint32 talentTabId = talentTabIds[i];

            for (uint32 talentId = 0; talentId < sTalentStore.GetNumRows(); ++talentId)
            {
                TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);
                if (!talentInfo)
                    continue;

                if (talentInfo->TalentTab != talentTabId)
                    continue;

                int8 curtalent_maxrank = -1;
                for (int8 rank = MAX_TALENT_RANK-1; rank >= 0; --rank)
                {
                    if (talentInfo->RankID[rank] && target->HasTalent(talentInfo->RankID[rank], target->GetActiveSpec()))
                    {
                        curtalent_maxrank = rank;
                        break;
                    }
                }

                if (curtalent_maxrank < 0)
                    continue;

                switch(i)
                {
                    case 0:     spec1 = spec1 + 1 + curtalent_maxrank;      break;
                    case 1:     spec2 = spec2 + 1 + curtalent_maxrank;      break;
                    case 2:     spec3 = spec3 + 1 + curtalent_maxrank;      break;
                    default:   continue;   break;
                }
            }
        }

        // Handling of spec name
        switch (target->getClass())
        {
            case CLASS_WARRIOR:
                clase = " Guerrero ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Tanke";
                    spec = "Proteccion";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Dps";
                    spec = "Furia";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Dps";
                    spec = "Armas";
                }
                break;
            case CLASS_PALADIN:
                clase = " Paladin ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Dps";
                    spec = "Reprension";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Tanke";
                    spec = "Proteccion";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Healer";
                    spec = "Sagrado";
                }
                break;
            case CLASS_HUNTER:
                clase = " Cazador ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Dps";
                    spec = "Supervivencia";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Dps";
                    spec = "Punteria";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Dps";
                    spec = "Bestias";
                }
                break;
            case CLASS_ROGUE:
                clase = " Picaro ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Dps";
                    spec = "Sutileza";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Dps";
                    spec = "Combate";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Dps";
                    spec = "Asesinato";
                }
                break;
            case CLASS_PRIEST:
                clase = " Sacerdote ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Dps";
                    spec = "Sombra";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Healer";
                    spec = "Sagrado";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Healer";
                    spec = "Disciplina";
                }
                break;
            case CLASS_DEATH_KNIGHT:
                clase = " Dk ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Dps";
                    spec = "Profano";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Tanke/Dps";
                    spec = "Escarcha";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Tanke/Dps";
                    spec = "Sangre";
                }
                break;
            case CLASS_SHAMAN:
                clase = " Chaman ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Healer";
                    spec = "Restauracion";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Dps";
                    spec = "Mejora";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Dps";
                    spec = "Elemental";
                }
                break;
            case CLASS_MAGE:
                clase = " Mago ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Dps";
                    spec = "Escarcha";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Dps";
                    spec = "Fuego";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Dps";
                    spec = "Arcano";
                }
                break;
            case CLASS_WARLOCK:
                clase = " Brujo ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Dps";
                    spec = "Destruccion";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Dps";
                    spec = "Demonologia";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Dps";
                    spec = "Afliccion";
                }
                break;
            case CLASS_DRUID:
                clase = " Druida ";
                if (spec3 > spec2 && spec3 > spec1)
                {
                    rol = "Healer";
                    spec = "Restauracion";
                }
                else if (spec2 > spec3 && spec2 > spec1)
                {
                    rol = "Tanke/Dps";
                    spec = "Feral";
                }
                else if (spec1 > spec2 && spec1 > spec3)
                {
                    rol = "Dps";
                    spec = "Equilibrio";
                }
                break;
        }

        // Gs & ilvl calculation
        gs = uint32(target->GetGearScore());
        ilvl = uint32(target->GetAverageItemLevel());

        // MMr pvp info
        for (uint8 slot = 0; slot < 3 ; ++slot)
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MATCH_MAKER_RATING);
            stmt->setUInt32(0, target->GetGUIDLow());
            stmt->setUInt8(1, slot);
            PreparedQueryResult result = CharacterDatabase.Query(stmt);
            if (!result)
                continue;

            switch (slot)
            {
                case 0:
                    mmrdos = (*result)[0].GetUInt16();
                    break;
                case 1:
                    mmrtres = (*result)[0].GetUInt16();
                    break;
                case 2:
                    mmrcinco = (*result)[0].GetUInt16();
                    break;
                default:
                    break;
            }
        }

        // Printing info
        if (handler->GetSession())
        {
            handler->PSendSysMessage(LANG_INFO_PLAYER_UPPER_BAR);
            handler->PSendSysMessage(LANG_INFO_PLAYER_NAME, name.c_str());
            handler->PSendSysMessage(LANG_INFO_PLAYER_GUILD, guild.c_str());
            handler->PSendSysMessage(LANG_INFO_PLAYER_CLASS_SPEC, clase.c_str(), spec.c_str());
            handler->PSendSysMessage(LANG_INFO_PLAYER_TALENT_DISTRIB, spec1, spec2, spec3);
            handler->PSendSysMessage(LANG_INFO_PLAYER_POSSIBLE_ROL, rol.c_str());
            handler->PSendSysMessage(LANG_INFO_PLAYER_TOTAL_GS, gs);
            handler->PSendSysMessage(LANG_INFO_PLAYER_EQUIPPED_ILVL, ilvl);
            handler->PSendSysMessage(LANG_INFO_PLAYER_MMR, mmrdos, mmrtres, mmrcinco);
            handler->PSendSysMessage(LANG_INFO_PLAYER_WOWREAN);
            handler->PSendSysMessage(LANG_INFO_PLAYER_LOWER_BAR);
        }

        return true;
    }

    static bool HandleRaidInfoCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        int32 instanceId = atoi((char*)args);

        if (instanceId < 1)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Group* group = sGroupMgr->GetGroupByInstanceId(instanceId);

        if (!group || !group->isRaidGroup())
            return false;

        Player* Leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());

        if (!Leader)
            return false;

        std::string lider(group->GetLeaderName());
        std::string instanceName("Indefinido");
        std::string difficulty("Indefinida");

        uint32 completedEncounters = 0;
        uint32 totalEncounters = 0;

        uint32 countPlayers = 0;
        uint8 totalPlayers = 0;
        uint8 rolTank = 0;
        uint8 rolHeal = 0;
        uint8 rolDps = 0;

        uint32 avgIlvl = 0;
        uint32 avgGs = 0;

        // Instance data handling, killed bosses, instance name and difficulty according to input id
        InstanceSave* instanceSave = sInstanceSaveMgr->GetInstanceSave(instanceId);
        if (!instanceSave)
            return false;

        Map const* map = sMapMgr->CreateBaseMap(instanceSave->GetMapId());
        if (!map)
            return false;

        Map* iMap = ((MapInstanced*)map)->FindInstanceMap(instanceId);
        if (!iMap)
            return false;

        InstanceScript* instance = ((InstanceMap*)iMap)->GetInstanceScript();
        if (!instance)
            return false;

        // Progression Handling, actual killed bosses and total bosses
        completedEncounters = instance->GetCompletedEncountersReadable();
        totalEncounters = instance->GetTotalEncountersReadable();

        // Map name, difficulty and total players
        instanceName = iMap->GetMapName();
        switch(iMap->GetDifficulty())
        {
            case RAID_DIFFICULTY_10MAN_NORMAL:
                if (iMap->IsNonRaidDungeon())
                {
                    difficulty = "5N";
                    totalPlayers = 5;
                }
                else
                {
                    difficulty = "10N";
                    totalPlayers = 10;
                }
                break;
            case RAID_DIFFICULTY_25MAN_NORMAL:
                if (iMap->IsNonRaidDungeon())
                {
                    difficulty = "5H";
                    totalPlayers = 5;
                }
                else
                {
                    difficulty = "25N";
                    totalPlayers = 25;
                }
                break;
            case RAID_DIFFICULTY_10MAN_HEROIC:
                if (iMap->IsNonRaidDungeon())
                {
                    difficulty = "5N";
                    totalPlayers = 5;
                }
                else
                {
                    difficulty = "10H";
                    totalPlayers = 10;
                }
                break;
            case RAID_DIFFICULTY_25MAN_HEROIC:
                if (iMap->IsNonRaidDungeon())
                {
                    difficulty = "5H";
                    totalPlayers = 5;
                }
                else
                {
                    difficulty = "25H";
                    totalPlayers = 25;
                }
                break;
            default:
                break;
        }

        // Raid composition and GS & ilvl calculation
        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* groupMember = itr->GetSource();
            if (!groupMember || !groupMember->GetSession())
                continue;

            if (!groupMember->IsInWorld())
                continue;

            if (groupMember->GetMapId() != iMap->GetId())
                continue;

            switch(groupMember->GetPossibleRole())
            {
                case CHARACTER_ROLE_TANK:
                    rolTank++;
                    break;
                case CHARACTER_ROLE_HEAL:
                    rolHeal++;
                    break;
                case CHARACTER_ROLE_DPS:
                    rolDps++;
                    break;
                case CHARACTER_ROLE_NONE:
                    break;
            }

            // Avg Gs & avg Ilvl
            avgGs = avgGs + groupMember->GetGearScore();
            avgIlvl = avgIlvl + groupMember->GetAverageItemLevel();

            // Counting players raid
            countPlayers++;
        }

        if (countPlayers != 0)
        {
            avgGs = uint32(avgGs / countPlayers);
            avgIlvl = uint32(avgIlvl / countPlayers);
        }
        else
        {
            avgGs = 0;
            avgIlvl = 0;
        }

        // Printing info
        if (handler->GetSession())
        {
            handler->PSendSysMessage(LANG_INFO_RAID_UPPER_BAR);
            handler->PSendSysMessage(LANG_INFO_RAID_ID, instanceId);
            handler->PSendSysMessage(LANG_INFO_RAID_NAME_DIFFICULTY, instanceName.c_str(), difficulty.c_str());
            handler->PSendSysMessage(LANG_INFO_RAID_PROGRESS, completedEncounters, totalEncounters);
            handler->PSendSysMessage(LANG_INFO_RAID_LEADER, lider.c_str());
            handler->PSendSysMessage(LANG_INFO_RAID_COMPOSITION, countPlayers, totalPlayers);
            handler->PSendSysMessage(LANG_INFO_RAID_ROLES, rolTank, rolHeal, rolDps);
            handler->PSendSysMessage(LANG_INFO_RAID_AVG_GS, avgGs);
            handler->PSendSysMessage(LANG_INFO_RAID_AVG_ILVL, avgIlvl);
            handler->PSendSysMessage(LANG_INFO_RAID_WOWREAN);
            handler->PSendSysMessage(LANG_INFO_RAID_LOWER_BAR);
        }

        return true;
    }

    static bool HandleRaidListCommand(ChatHandler* handler, char const* args)
    {
        Player* player;
        if (!handler->extractPlayerTarget((char*)args, &player))
            return false;

        if (!player)
            return false;

        std::string instanceName("Indefinido");
        std::string difficulty("Indefinida");
        std::string playerName(player->GetName());

        handler->PSendSysMessage(LANG_LIST_RAID_UPPER_BAR);
        handler->PSendSysMessage(LANG_LIST_RAID_PLAYER, playerName.c_str());

        for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        {
            Player::BoundInstancesMap &binds = player->GetBoundInstances(Difficulty(i));
            for (Player::BoundInstancesMap::const_iterator itr = binds.begin(); itr != binds.end(); ++itr)
            {
                InstanceSave* instanceSave = itr->second.save;
                instanceName = instanceSave->GetMapEntry()->name[sWorld->GetDefaultDbcLocale()];

                switch(instanceSave->GetDifficulty())
                {
                    case RAID_DIFFICULTY_10MAN_NORMAL:
                        if (instanceSave->GetMapEntry()->IsNonRaidDungeon())
                            difficulty = "5N";
                        else
                            difficulty = "10N";
                        break;
                    case RAID_DIFFICULTY_25MAN_NORMAL:
                        if (instanceSave->GetMapEntry()->IsNonRaidDungeon())
                            difficulty = "5H";
                        else
                            difficulty = "25N";
                        break;
                    case RAID_DIFFICULTY_10MAN_HEROIC:
                        if (instanceSave->GetMapEntry()->IsNonRaidDungeon())
                            difficulty = "5N";
                        else
                            difficulty = "10H";
                        break;
                    case RAID_DIFFICULTY_25MAN_HEROIC:
                        if (instanceSave->GetMapEntry()->IsNonRaidDungeon())
                            difficulty = "5H";
                        else
                            difficulty = "25H";
                        break;
                    default:
                        break;
                }

                if (player->GetMapId() == instanceSave->GetMapId() && player->GetMap()->GetDifficulty() == instanceSave->GetDifficulty())
                    handler->PSendSysMessage(LANG_LIST_RAID_ACTIVE, instanceSave->GetInstanceId(), instanceName.c_str(), difficulty.c_str());
                else
                    handler->PSendSysMessage(LANG_LIST_RAID_INACTIVE, instanceSave->GetInstanceId(), instanceName.c_str(), difficulty.c_str());
            }
        }

        handler->PSendSysMessage(LANG_LIST_RAID_WOWREAN);
        handler->PSendSysMessage(LANG_LIST_RAID_LOWER_BAR);

        return true;
    }
};

void AddSC_info_commandscript()
{
    new info_commandscript();
}
