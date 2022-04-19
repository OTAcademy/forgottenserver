// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "events.h"
#include "item.h"
#include "monster.h"
#include "player.h"
#include "tools.h"

Events::Events() :
	scriptInterface("Event Interface")
{
	scriptInterface.initState();
}

bool Events::load()
{
	std::string location = "Events::load";

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/events/events.xml");
	if (!result) {
		printXMLError(location, "data/events/events.xml", result);
		return false;
	}

	info = {};

	std::set<std::string> classes;
	for (auto eventNode : doc.child("events").children()) {
		if (!eventNode.attribute("enabled").as_bool()) {
			continue;
		}

		const std::string& className = eventNode.attribute("class").as_string();
		auto res = classes.insert(className);
		if (res.second) {
			const std::string& lowercase = asLowerCaseString(className);
			if (scriptInterface.loadFile("data/events/scripts/" + lowercase + ".lua") != 0) {
				console::reportFileError(location, lowercase, scriptInterface.getLastLuaError());
			}
		}

		const std::string& methodName = eventNode.attribute("method").as_string();
		const int32_t event = scriptInterface.getMetaEvent(className, methodName);
		if (className == "Creature") {
			if (methodName == "onChangeOutfit") {
				info.creatureOnChangeOutfit = event;
			} else if (methodName == "onAreaCombat") {
				info.creatureOnAreaCombat = event;
			} else if (methodName == "onTargetCombat") {
				info.creatureOnTargetCombat = event;
			} else if (methodName == "onHear") {
				info.creatureOnHear = event;
			} else {
				console::reportWarning(location, "Unknown creature method \"" + methodName + "\"!");
			}
		} else if (className == "Party") {
			if (methodName == "onJoin") {
				info.partyOnJoin = event;
			} else if (methodName == "onLeave") {
				info.partyOnLeave = event;
			} else if (methodName == "onDisband") {
				info.partyOnDisband = event;
			} else if (methodName == "onShareExperience") {
				info.partyOnShareExperience = event;
			} else {
				console::reportWarning(location, "Unknown party method \"" + methodName + "\"!");
			}
		} else if (className == "Player") {
			if (methodName == "onBrowseField") {
				info.playerOnBrowseField = event;
			} else if (methodName == "onLook") {
				info.playerOnLook = event;
			} else if (methodName == "onLookInBattleList") {
				info.playerOnLookInBattleList = event;
			} else if (methodName == "onLookInTrade") {
				info.playerOnLookInTrade = event;
			} else if (methodName == "onLookInShop") {
				info.playerOnLookInShop = event;
			} else if (methodName == "onLookInMarket") {
				info.playerOnLookInMarket = event;
			} else if (methodName == "onTradeRequest") {
				info.playerOnTradeRequest = event;
			} else if (methodName == "onTradeAccept") {
				info.playerOnTradeAccept = event;
			} else if (methodName == "onTradeCompleted") {
				info.playerOnTradeCompleted = event;
			} else if (methodName == "onPodiumRequest") {
				info.playerOnPodiumRequest = event;
			} else if (methodName == "onPodiumEdit") {
				info.playerOnPodiumEdit = event;
			} else if (methodName == "onMoveItem") {
				info.playerOnMoveItem = event;
			} else if (methodName == "onItemMoved") {
				info.playerOnItemMoved = event;
			} else if (methodName == "onMoveCreature") {
				info.playerOnMoveCreature = event;
			} else if (methodName == "onReportRuleViolation") {
				info.playerOnReportRuleViolation = event;
			} else if (methodName == "onReportBug") {
				info.playerOnReportBug = event;
			} else if (methodName == "onTurn") {
				info.playerOnTurn = event;
			} else if (methodName == "onGainExperience") {
				info.playerOnGainExperience = event;
			} else if (methodName == "onLoseExperience") {
				info.playerOnLoseExperience = event;
			} else if (methodName == "onGainSkillTries") {
				info.playerOnGainSkillTries = event;
			} else if (methodName == "onWrapItem") {
				info.playerOnWrapItem = event;
			} else if (methodName == "onInspectItem") {
				info.playerOnInspectItem = event;
			} else if (methodName == "onInspectTradeItem") {
				info.playerOnInspectTradeItem = event;
			} else if (methodName == "onInspectNpcTradeItem") {
				info.playerOnInspectNpcTradeItem = event;
			} else if (methodName == "onInspectCompendiumItem") {
				info.playerOnInspectCompendiumItem = event;
			} else if (methodName == "onMinimapQuery") {
				info.playerOnMinimapQuery = event;
			} else if (methodName == "onInventoryUpdate") {
				info.playerOnInventoryUpdate = event;
			} else if (methodName == "onConnect") {
				info.playerOnConnect = event;
			} else if (methodName == "onNetworkMessage") {
				info.playerOnNetworkMessage = event;
			} else {
				console::reportWarning(location, "Unknown player method \"" + methodName + "\"!");
			}
		} else if (className == "Monster") {
			if (methodName == "onDropLoot") {
				info.monsterOnDropLoot = event;
			} else if (methodName == "onSpawn") {
				info.monsterOnSpawn = event;
			} else {
				console::reportWarning(location, "Unknown monster method \"" + methodName + "\"!");
			}
		} else {
			console::reportWarning(location, "Unknown class \"" + className + "\"!");
		}
	}
	return true;
}

// Monster
bool Events::eventMonsterOnSpawn(Monster* monster, const Position& position, bool startup, bool artificial)
{
	// Monster:onSpawn(position, startup, artificial)
	if (info.monsterOnSpawn == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::monsterOnSpawn");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.monsterOnSpawn, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.monsterOnSpawn);

	// set monster ID earlier than usual so we can reference it in the script
	monster->setID();

	LuaScriptInterface::pushUserdata<Monster>(L, monster);
	LuaScriptInterface::setMetatable(L, -1, "Monster");
	LuaScriptInterface::pushPosition(L, position);
	LuaScriptInterface::pushBoolean(L, startup);
	LuaScriptInterface::pushBoolean(L, artificial);

	return scriptInterface.callFunction(4);
}

// Creature
bool Events::eventCreatureOnChangeOutfit(Creature* creature, const Outfit_t& outfit)
{
	// Creature:onChangeOutfit(outfit)
	if (info.creatureOnChangeOutfit == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventCreatureOnChangeOutfit");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.creatureOnChangeOutfit, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.creatureOnChangeOutfit);

	LuaScriptInterface::pushUserdata<Creature>(L, creature);
	LuaScriptInterface::setCreatureMetatable(L, -1, creature);

	LuaScriptInterface::pushOutfit(L, outfit);

	return scriptInterface.callFunction(2);
}

ReturnValue Events::eventCreatureOnAreaCombat(Creature* creature, Tile* tile, bool aggressive)
{
	// Creature:onAreaCombat(tile, aggressive)
	if (info.creatureOnAreaCombat == -1) {
		return RETURNVALUE_NOERROR;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventCreatureOnAreaCombat");
		return RETURNVALUE_NOTPOSSIBLE;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.creatureOnAreaCombat, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.creatureOnAreaCombat);

	if (creature) {
		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);
	} else {
		lua_pushnil(L);
	}

	LuaScriptInterface::pushUserdata<Tile>(L, tile);
	LuaScriptInterface::setMetatable(L, -1, "Tile");

	LuaScriptInterface::pushBoolean(L, aggressive);

	ReturnValue returnValue;
	if (scriptInterface.protectedCall(L, 3, 1) != 0) {
		returnValue = RETURNVALUE_NOTPOSSIBLE;
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		returnValue = LuaScriptInterface::getNumber<ReturnValue>(L, -1);
		lua_pop(L, 1);
	}

	scriptInterface.resetScriptEnv();
	return returnValue;
}

ReturnValue Events::eventCreatureOnTargetCombat(Creature* creature, Creature* target)
{
	// Creature:onTargetCombat(target)
	if (info.creatureOnTargetCombat == -1) {
		return RETURNVALUE_NOERROR;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventCreatureOnTargetCombat");
		return RETURNVALUE_NOTPOSSIBLE;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.creatureOnTargetCombat, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.creatureOnTargetCombat);

	if (creature) {
		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);
	} else {
		lua_pushnil(L);
	}

	LuaScriptInterface::pushUserdata<Creature>(L, target);
	LuaScriptInterface::setCreatureMetatable(L, -1, target);

	ReturnValue returnValue;
	if (scriptInterface.protectedCall(L, 2, 1) != 0) {
		returnValue = RETURNVALUE_NOTPOSSIBLE;
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		returnValue = LuaScriptInterface::getNumber<ReturnValue>(L, -1);
		lua_pop(L, 1);
	}

	scriptInterface.resetScriptEnv();
	return returnValue;
}

void Events::eventCreatureOnHear(Creature* creature, Creature* speaker, const std::string& words, MessageClasses type)
{
	// Creature:onHear(speaker, words, type)
	if (info.creatureOnHear == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventCreatureOnHear");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.creatureOnHear, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.creatureOnHear);

	LuaScriptInterface::pushUserdata<Creature>(L, creature);
	LuaScriptInterface::setCreatureMetatable(L, -1, creature);

	LuaScriptInterface::pushUserdata<Creature>(L, speaker);
	LuaScriptInterface::setCreatureMetatable(L, -1, speaker);

	LuaScriptInterface::pushString(L, words);
	lua_pushnumber(L, type);

	scriptInterface.callVoidFunction(4);
}

// Party
bool Events::eventPartyOnJoin(Party* party, Player* player)
{
	// Party:onJoin(player)
	if (info.partyOnJoin == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPartyOnJoin");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.partyOnJoin, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.partyOnJoin);

	LuaScriptInterface::pushUserdata<Party>(L, party);
	LuaScriptInterface::setMetatable(L, -1, "Party");

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	return scriptInterface.callFunction(2);
}

bool Events::eventPartyOnLeave(Party* party, Player* player)
{
	// Party:onLeave(player)
	if (info.partyOnLeave == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPartyOnLeave");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.partyOnLeave, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.partyOnLeave);

	LuaScriptInterface::pushUserdata<Party>(L, party);
	LuaScriptInterface::setMetatable(L, -1, "Party");

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	return scriptInterface.callFunction(2);
}

bool Events::eventPartyOnDisband(Party* party)
{
	// Party:onDisband()
	if (info.partyOnDisband == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPartyOnDisband");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.partyOnDisband, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.partyOnDisband);

	LuaScriptInterface::pushUserdata<Party>(L, party);
	LuaScriptInterface::setMetatable(L, -1, "Party");

	return scriptInterface.callFunction(1);
}

void Events::eventPartyOnShareExperience(Party* party, uint64_t& exp)
{
	// Party:onShareExperience(exp)
	if (info.partyOnShareExperience == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPartyOnShareExperience");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.partyOnShareExperience, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.partyOnShareExperience);

	LuaScriptInterface::pushUserdata<Party>(L, party);
	LuaScriptInterface::setMetatable(L, -1, "Party");

	lua_pushnumber(L, exp);

	if (scriptInterface.protectedCall(L, 2, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		exp = LuaScriptInterface::getNumber<uint64_t>(L, -1);
		lua_pop(L, 1);
	}

	scriptInterface.resetScriptEnv();
}

// Player
bool Events::eventPlayerOnBrowseField(Player* player, const Position& position)
{
	// Player:onBrowseField(position)
	if (info.playerOnBrowseField == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnBrowseField");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnBrowseField, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnBrowseField);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushPosition(L, position);

	return scriptInterface.callFunction(2);
}

void Events::eventPlayerOnLook(Player* player, const Position& position, Thing* thing, uint8_t stackpos, int32_t lookDistance)
{
	// Player:onLook(thing, position, distance)
	if (info.playerOnLook == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnLook");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnLook, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnLook);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	if (Creature* creature = thing->getCreature()) {
		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);
	} else if (Item* item = thing->getItem()) {
		LuaScriptInterface::pushUserdata<Item>(L, item);
		LuaScriptInterface::setItemMetatable(L, -1, item);
	} else {
		lua_pushnil(L);
	}

	LuaScriptInterface::pushPosition(L, position, stackpos);
	lua_pushnumber(L, lookDistance);

	scriptInterface.callVoidFunction(4);
}

void Events::eventPlayerOnLookInBattleList(Player* player, Creature* creature, int32_t lookDistance)
{
	// Player:onLookInBattleList(creature, position, distance)
	if (info.playerOnLookInBattleList == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnLookInBattleList");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnLookInBattleList, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnLookInBattleList);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Creature>(L, creature);
	LuaScriptInterface::setCreatureMetatable(L, -1, creature);

	lua_pushnumber(L, lookDistance);

	scriptInterface.callVoidFunction(3);
}

void Events::eventPlayerOnLookInTrade(Player* player, Player* partner, Item* item, int32_t lookDistance)
{
	// Player:onLookInTrade(partner, item, distance)
	if (info.playerOnLookInTrade == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnLookInTrade");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnLookInTrade, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnLookInTrade);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Player>(L, partner);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	lua_pushnumber(L, lookDistance);

	scriptInterface.callVoidFunction(4);
}

bool Events::eventPlayerOnLookInShop(Player* player, const ItemType* itemType, uint8_t count, Npc* npc)
{
	// Player:onLookInShop(itemType, count, npc)
	if (info.playerOnLookInShop == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnLookInShop");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnLookInShop, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnLookInShop);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<const ItemType>(L, itemType);
	LuaScriptInterface::setMetatable(L, -1, "ItemType");

	lua_pushnumber(L, count);

	LuaScriptInterface::pushUserdata<Npc>(L, npc);
	LuaScriptInterface::setMetatable(L, -1, "Npc");

	return scriptInterface.callFunction(4);
}

bool Events::eventPlayerOnLookInMarket(Player* player, const ItemType* itemType, uint8_t tier)
{
	// Player:onLookInMarket(itemType, tier)
	if (info.playerOnLookInMarket == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnLookInMarket");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnLookInMarket, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnLookInMarket);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<const ItemType>(L, itemType);
	LuaScriptInterface::setMetatable(L, -1, "ItemType");

	lua_pushnumber(L, tier);

	return scriptInterface.callFunction(3);
}

ReturnValue Events::eventPlayerOnMoveItem(Player* player, Item* item, uint16_t count, const Position& fromPosition, const Position& toPosition, Cylinder* fromCylinder, Cylinder* toCylinder)
{
	// Player:onMoveItem(item, count, fromPosition, toPosition, fromCylinder, toCylinder)
	if (info.playerOnMoveItem == -1) {
		return RETURNVALUE_NOERROR;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnMoveItem");
		return RETURNVALUE_NOTPOSSIBLE;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnMoveItem, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnMoveItem);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	lua_pushnumber(L, count);
	LuaScriptInterface::pushPosition(L, fromPosition);
	LuaScriptInterface::pushPosition(L, toPosition);

	LuaScriptInterface::pushCylinder(L, fromCylinder);
	LuaScriptInterface::pushCylinder(L, toCylinder);

	ReturnValue returnValue;
	if (scriptInterface.protectedCall(L, 7, 1) != 0) {
		returnValue = RETURNVALUE_NOTPOSSIBLE;
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		returnValue = LuaScriptInterface::getNumber<ReturnValue>(L, -1);
		lua_pop(L, 1);
	}

	scriptInterface.resetScriptEnv();
	return returnValue;
}

void Events::eventPlayerOnItemMoved(Player* player, Item* item, uint16_t count, const Position& fromPosition, const Position& toPosition, Cylinder* fromCylinder, Cylinder* toCylinder)
{
	// Player:onItemMoved(item, count, fromPosition, toPosition, fromCylinder, toCylinder)
	if (info.playerOnItemMoved == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnItemMoved");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnItemMoved, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnItemMoved);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	if (item) {
		LuaScriptInterface::pushUserdata<Item>(L, item);
		LuaScriptInterface::setItemMetatable(L, -1, item);
	} else {
		lua_pushnil(L);
	}

	lua_pushnumber(L, count);
	LuaScriptInterface::pushPosition(L, fromPosition);
	LuaScriptInterface::pushPosition(L, toPosition);

	LuaScriptInterface::pushCylinder(L, fromCylinder);
	LuaScriptInterface::pushCylinder(L, toCylinder);

	scriptInterface.callVoidFunction(7);
}

bool Events::eventPlayerOnMoveCreature(Player* player, Creature* creature, const Position& fromPosition, const Position& toPosition)
{
	// Player:onMoveCreature(creature, fromPosition, toPosition)
	if (info.playerOnMoveCreature == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnMoveCreature");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnMoveCreature, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnMoveCreature);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Creature>(L, creature);
	LuaScriptInterface::setCreatureMetatable(L, -1, creature);

	LuaScriptInterface::pushPosition(L, fromPosition);
	LuaScriptInterface::pushPosition(L, toPosition);

	return scriptInterface.callFunction(4);
}

void Events::eventPlayerOnReportRuleViolation(Player* player, const std::string& targetName, uint8_t reportType, uint8_t reportReason, const std::string& comment, const std::string& translation)
{
	// Player:onReportRuleViolation(targetName, reportType, reportReason, comment, translation)
	if (info.playerOnReportRuleViolation == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnReportRuleViolation");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnReportRuleViolation, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnReportRuleViolation);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushString(L, targetName);

	lua_pushnumber(L, reportType);
	lua_pushnumber(L, reportReason);

	LuaScriptInterface::pushString(L, comment);
	LuaScriptInterface::pushString(L, translation);

	scriptInterface.callVoidFunction(6);
}

bool Events::eventPlayerOnReportBug(Player* player, const std::string& message, const Position& position, uint8_t category)
{
	// Player:onReportBug(message, position, category)
	if (info.playerOnReportBug == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnReportBug");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnReportBug, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnReportBug);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushString(L, message);
	LuaScriptInterface::pushPosition(L, position);
	lua_pushnumber(L, category);

	return scriptInterface.callFunction(4);
}

bool Events::eventPlayerOnTurn(Player* player, Direction direction)
{
	// Player:onTurn(direction)
	if (info.playerOnTurn == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnTurn");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnTurn, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnTurn);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	lua_pushnumber(L, direction);

	return scriptInterface.callFunction(2);
}

bool Events::eventPlayerOnTradeRequest(Player* player, Player* target, Item* item)
{
	// Player:onTradeRequest(target, item)
	if (info.playerOnTradeRequest == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnTradeRequest");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnTradeRequest, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnTradeRequest);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Player>(L, target);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	return scriptInterface.callFunction(3);
}

bool Events::eventPlayerOnTradeAccept(Player* player, Player* target, Item* item, Item* targetItem)
{
	// Player:onTradeAccept(target, item, targetItem)
	if (info.playerOnTradeAccept == -1) {
		return true;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnTradeAccept");
		return false;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnTradeAccept, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnTradeAccept);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Player>(L, target);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	LuaScriptInterface::pushUserdata<Item>(L, targetItem);
	LuaScriptInterface::setItemMetatable(L, -1, targetItem);

	return scriptInterface.callFunction(4);
}

void Events::eventPlayerOnTradeCompleted(Player* player, Player* target, Item* item, Item* targetItem, bool isSuccess)
{
	// Player:onTradeCompleted(target, item, targetItem, isSuccess)
	if (info.playerOnTradeCompleted == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnTradeCompleted");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnTradeCompleted, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnTradeCompleted);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Player>(L, target);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	LuaScriptInterface::pushUserdata<Item>(L, targetItem);
	LuaScriptInterface::setItemMetatable(L, -1, targetItem);

	LuaScriptInterface::pushBoolean(L, isSuccess);

	return scriptInterface.callVoidFunction(5);
}

void Events::eventPlayerOnPodiumRequest(Player* player, Item* item)
{
	// Player:onPodiumRequest(item)
	if (info.playerOnPodiumRequest == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnPodiumRequest");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnPodiumRequest, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnPodiumRequest);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	scriptInterface.callFunction(2);
}

void Events::eventPlayerOnPodiumEdit(Player* player, Item* item, const Outfit_t& outfit, bool podiumVisible, Direction direction)
{
	// Player:onPodiumEdit(item, outfit, direction, isVisible)
	if (info.playerOnPodiumEdit == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnPodiumEdit");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnPodiumEdit, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnPodiumEdit);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	LuaScriptInterface::pushOutfit(L, outfit);

	lua_pushnumber(L, direction);
	lua_pushboolean(L, podiumVisible);

	scriptInterface.callFunction(5);
}

void Events::eventPlayerOnGainExperience(Player* player, Creature* source, uint64_t& exp, uint64_t rawExp)
{
	// Player:onGainExperience(source, exp, rawExp)
	// rawExp gives the original exp which is not multiplied
	if (info.playerOnGainExperience == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnGainExperience");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnGainExperience, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnGainExperience);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}

	lua_pushnumber(L, exp);
	lua_pushnumber(L, rawExp);

	if (scriptInterface.protectedCall(L, 4, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		exp = LuaScriptInterface::getNumber<uint64_t>(L, -1);
		lua_pop(L, 1);
	}

	scriptInterface.resetScriptEnv();
}

void Events::eventPlayerOnLoseExperience(Player* player, uint64_t& exp)
{
	// Player:onLoseExperience(exp)
	if (info.playerOnLoseExperience == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnLoseExperience");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnLoseExperience, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnLoseExperience);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	lua_pushnumber(L, exp);

	if (scriptInterface.protectedCall(L, 2, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		exp = LuaScriptInterface::getNumber<uint64_t>(L, -1);
		lua_pop(L, 1);
	}

	scriptInterface.resetScriptEnv();
}

void Events::eventPlayerOnGainSkillTries(Player* player, skills_t skill, uint64_t& tries)
{
	// Player:onGainSkillTries(skill, tries)
	if (info.playerOnGainSkillTries == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnGainSkillTries");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnGainSkillTries, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnGainSkillTries);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	lua_pushnumber(L, skill);
	lua_pushnumber(L, tries);

	if (scriptInterface.protectedCall(L, 3, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		tries = LuaScriptInterface::getNumber<uint64_t>(L, -1);
		lua_pop(L, 1);
	}

	scriptInterface.resetScriptEnv();
}

void Events::eventPlayerOnWrapItem(Player* player, Item* item)
{
	// Player:onWrapItem(item)
	if (info.playerOnWrapItem == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnWrapItem");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnWrapItem, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnWrapItem);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	scriptInterface.callVoidFunction(2);
}

void Events::eventPlayerOnInspectItem(Player* player, Item* item)
{
	// Player:onInspectItem(item)
	if (info.playerOnInspectItem == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnInspectItem");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnInspectItem, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnInspectItem);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	scriptInterface.callVoidFunction(2);
}

void Events::eventPlayerOnInspectTradeItem(Player* player, Player* tradePartner, Item* item)
{
	// Player:onInspectTradeItem(tradePartner, item)
	if (info.playerOnInspectTradeItem == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::onInspectTradeItem");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnInspectTradeItem, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnInspectTradeItem);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Player>(L, tradePartner);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	scriptInterface.callVoidFunction(3);
}

void Events::eventPlayerOnInspectNpcTradeItem(Player* player, Npc* npc, uint16_t itemId)
{
	// Player:onInspectNpcTradeItem(npc, itemId)
	if (info.playerOnInspectNpcTradeItem == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::onInspectNpcTradeItem");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnInspectNpcTradeItem, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnInspectNpcTradeItem);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Npc>(L, npc);
	LuaScriptInterface::setMetatable(L, -1, "Npc");

	lua_pushnumber(L, itemId);

	scriptInterface.callVoidFunction(3);
}

void Events::eventPlayerOnInspectCompendiumItem(Player* player, uint16_t itemId)
{
	// Player:onInspectCompendiumItem(itemId)
	if (info.playerOnInspectCompendiumItem == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::onInspectCompendiumItem");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnInspectCompendiumItem, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnInspectCompendiumItem);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	lua_pushnumber(L, itemId);

	scriptInterface.callVoidFunction(2);
}

void Events::eventPlayerOnMinimapQuery(Player* player, const Position& position)
{
	// Player:onMinimapQuery(position)
	if (info.playerOnMinimapQuery == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::onMinimapQuery");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnMinimapQuery, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnMinimapQuery);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushPosition(L, position);

	scriptInterface.callVoidFunction(2);
}

void Events::eventPlayerOnInventoryUpdate(Player* player, Item* item, slots_t slot, bool equip)
{
	// Player:onInventoryUpdate(item, slot, equip)
	if (info.playerOnInventoryUpdate == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		std::cout << "[Error - Events::eventPlayerOnInventoryUpdate] Call stack overflow" << std::endl;
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnInventoryUpdate, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnInventoryUpdate);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	LuaScriptInterface::pushUserdata<Item>(L, item);
	LuaScriptInterface::setItemMetatable(L, -1, item);

	lua_pushnumber(L, slot);
	LuaScriptInterface::pushBoolean(L, equip);

	scriptInterface.callVoidFunction(4);
}

void Events::eventPlayerOnConnect(Player* player, bool isLogin)
{
	// Player:onConnect(isLogin)
	if (info.playerOnConnect == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventPlayerOnConnect");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnConnect, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnConnect);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	lua_pushboolean(L, isLogin);
	scriptInterface.callVoidFunction(2);
}

void Events::eventPlayerOnNetworkMessage(Player* player, uint8_t recvByte, NetworkMessage* msg)
{
	// Player:onNetworkMessage(recvByte, msg)
	if (info.playerOnNetworkMessage == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		std::cout << "[Error - Events::eventPlayerOnInventoryUpdate] Call stack overflow" << std::endl;
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.playerOnNetworkMessage, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.playerOnNetworkMessage);

	LuaScriptInterface::pushUserdata<Player>(L, player);
	LuaScriptInterface::setMetatable(L, -1, "Player");

	lua_pushnumber(L, recvByte);

	LuaScriptInterface::pushUserdata<NetworkMessage>(L, msg);
	LuaScriptInterface::setMetatable(L, -1, "NetworkMessage");

	scriptInterface.callVoidFunction(3);
}

void Events::eventMonsterOnDropLoot(Monster* monster, Container* corpse)
{
	// Monster:onDropLoot(corpse)
	if (info.monsterOnDropLoot == -1) {
		return;
	}

	if (!scriptInterface.reserveScriptEnv()) {
		console::reportOverflow("Events::eventMonsterOnDropLoot");
		return;
	}

	ScriptEnvironment* env = scriptInterface.getScriptEnv();
	env->setScriptId(info.monsterOnDropLoot, &scriptInterface);

	lua_State* L = scriptInterface.getLuaState();
	scriptInterface.pushFunction(info.monsterOnDropLoot);

	LuaScriptInterface::pushUserdata<Monster>(L, monster);
	LuaScriptInterface::setMetatable(L, -1, "Monster");

	LuaScriptInterface::pushUserdata<Container>(L, corpse);
	LuaScriptInterface::setMetatable(L, -1, "Container");

	return scriptInterface.callVoidFunction(2);
}

