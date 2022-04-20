if not AutoLoot then
	AutoLoot = {}
end

-- 143 - loot corpse/tile
AUTOLOOT_REQUEST_QUICKLOOT = 0x8F

-- 145 - add/remove loot item
AUTOLOOT_REQUEST_SETSETTINGS = 0x91

LOOTED_RESOURCE_ABSENT = 0 -- corpse has no items
LOOTED_RESOURCE_NONE = 1 -- failed to loot items
LOOTED_RESOURCE_SOME = 2 -- failed to loot some items
LOOTED_RESOURCE_ALL = 3 -- looted all items

local config = {
	maxCorpsesLimit = 20, -- how many corpses will be checked
	maxLootListLength = 2000, -- how many items can the player register
	messageErrorPosition = "You cannot loot this position.",
	messageErrorOwner = "You are not the owner.",
	
	lootAbsent = "No loot.",
	lootStart = "You looted",
	lootItem = {
		[LOOTED_RESOURCE_NONE] = "none of the dropped items",
		[LOOTED_RESOURCE_SOME] = "only some of the dropped items",
		[LOOTED_RESOURCE_ALL] = "all items",
	},
	
	lootGold = {
		[LOOTED_RESOURCE_NONE] = "none of the dropped gold",
		[LOOTED_RESOURCE_SOME] = "only some of the dropped gold",
		[LOOTED_RESOURCE_ALL] = "complete %d gold",
	},
}

local function getLootedStatus(currentAmount, totalAmount, currentStatus)
	if currentAmount == 0 and totalAmount == 0 then
		return LOOTED_RESOURCE_ABSENT
	elseif currentAmount == 0 then
		return LOOTED_RESOURCE_NONE
	elseif currentAmount < totalAmount then
		return LOOTED_RESOURCE_SOME
	end
	
	return LOOTED_RESOURCE_ALL
end

local function getNextLootedStatus(currentStatus, elementStatus)
	if elementStatus == LOOTED_RESOURCE_ABSENT then
		return currentStatus 
	elseif currentStatus == elementStatus or currentStatus == LOOTED_RESOURCE_ABSENT then
		return elementStatus
	elseif currentStatus == LOOTED_RESOURCE_ALL or currentStatus == LOOTED_RESOURCE_NONE and (elementStatus == LOOTED_RESOURCE_ALL or elementStatus == LOOTED_RESOURCE_SOME) then
		return LOOTED_RESOURCE_SOME
	end
	
	return currentStatus
end

local function getLootResponse(lootedItems, lootedGold, itemCount, goldCount, corpseCount)
	if lootedItems == LOOTED_RESOURCE_ABSENT and lootedGold == LOOTED_RESOURCE_ABSENT then
		if corpseCount > 1 then
			return string.format("%s (%d corpse%s)", config.lootAbsent, corpseCount, corpseCount == 1 and "" or "s")
		else
			return config.lootAbsent
		end
	elseif
		corpseCount > 1 and (lootedItems == LOOTED_RESOURCE_ALL and lootedGold == LOOTED_RESOURCE_ALL
		or lootedItems == LOOTED_RESOURCE_ALL and lootedGold == LOOTED_RESOURCE_ABSENT
		or lootedItems == LOOTED_RESOURCE_ABSENT and lootedGold == LOOTED_RESOURCE_ALL)
	then
		return string.format("%s %d corpse%s.", config.lootStart, corpseCount, corpseCount == 1 and "" or "s")
	end
	
	local lootInfo = {}
	if lootedItems ~= LOOTED_RESOURCE_ABSENT then
		if itemCount == 1 then
			lootInfo[#lootInfo + 1] = "1 item"
		else
			lootInfo[#lootInfo + 1] = config.lootItem[lootedItems]
		end
	end
	
	if lootedGold ~= LOOTED_RESOURCE_ABSENT then
		if lootedGold == LOOTED_RESOURCE_ALL then
			lootInfo[#lootInfo + 1] = string.format(config.lootGold[lootedGold], goldCount)
		else
			lootInfo[#lootInfo + 1] = config.lootGold[lootedGold]
		end
	end
	
	local corpses = ""
	if corpseCount > 1 then
		corpses = string.format(" (%d corpse%s)", corpseCount, corpseCount == 1 and "" or "s")
	end
	
	return string.format("%s %s.%s", config.lootStart, table.concat(lootInfo, " and "), corpses)
end

function internalLootCorpse(player, corpse, lootedItems, lootedGold)
	if not corpse:isContainer() then
		return LOOTED_RESOURCE_ABSENT, LOOTED_RESOURCE_ABSENT, 0, 0
	end
	
	local corpseItems = 0
	local retrievedItems = 0
	local corpseGold = 0
	local retrievedGold = 0 -- stacks
	local retrievedGoldAmount = 0 -- exact amount
	
	for _, corpseItem in pairs(corpse:getItems()) do
		local isCurrency = corpseItem:isCurrency()
		if isCurrency then
			corpseGold = corpseGold + 1
		else
			corpseItems = corpseItems + 1
		end
		
		local lootedItem = corpseItem:clone()
		if player:addItemEx(lootedItem) == RETURNVALUE_NOERROR then
			corpseItem:remove()
			
			if isCurrency then
				retrievedGold = retrievedGold + 1
				retrievedGoldAmount = retrievedGoldAmount + corpseItem:getWorth()
			else
				retrievedItems = retrievedItems + 1
			end
		else
			lootedItem:remove()
		end
	end
	
	-- looted items response
	return getLootedStatus(retrievedItems, corpseItems, lootedItems), getLootedStatus(retrievedGold, corpseGold, lootedGold), retrievedItems, retrievedGoldAmount
end

function parseRequestQuickLoot(player, recvbyte, msg)
	local position = Position(msg:getU16(), msg:getU16(), msg:getByte())	
	
	local stackpos = msg:getByte()
	local spriteId = msg:getU16()
	local containerPos = msg:getByte()
	local isGround = msg:getByte() == 1

	local lootedItems = LOOTED_RESOURCE_ABSENT
	local lootedGold = LOOTED_RESOURCE_ABSENT
	local itemCount = 0
	local goldCount = 0
	local corpseCount = 0
	
	if position.x ~= CONTAINER_POSITION then
		-- shift + right click on the floor
	
		-- distance check
		if position:getDistance(player:getPosition()) > 1 then
			player:sendTextMessage(MESSAGE_LOOT, config.messageErrorPosition)
			return
		end

		-- tile check
		local tile = Tile(position)
		if not tile then
			player:sendTextMessage(MESSAGE_LOOT, config.messageErrorPosition)
			return
		end
		
		if tile:getHouse() then
			-- no looting inside houses
			return
		end
		
		local hasBodies = false
		local looted = false

		local items = tile:getItems()
		for _, corpse in ipairs(items) do
			if corpse:isCorpse() and corpse:isContainer() then
				hasBodies = true
			
				local owner = corpse:getCorpseOwner()
				local lootable = false
				if owner == player:getId() or owner == 0 then
					lootable = true
				else
					owner = Player(owner)
					if owner then
						local playerParty = player:getParty()
						local ownerParty = owner:getParty()
						if playerParty and ownerParty and playerParty == ownerParty then
							lootable = true
						end
					else
						lootable = true
					end
				end
				
				if lootable and corpseCount < config.maxCorpsesLimit then
					local tmpLootedItems = LOOTED_RESOURCE_ABSENT
					local tmpLootedGold = LOOTED_RESOURCE_ABSENT
					local tmpItemCount = 0
					local tmpGoldCount = 0
					
					tmpLootedItems, tmpLootedGold, tmpItemCount, tmpGoldCount = internalLootCorpse(player, corpse, tmpLootedItems, tmpLootedGold)
					corpseCount = corpseCount + 1
					lootedItems = getNextLootedStatus(lootedItems, tmpLootedItems)
					lootedGold = getNextLootedStatus(lootedGold, tmpLootedGold)
					itemCount = itemCount + tmpItemCount
					goldCount = goldCount + tmpGoldCount
					looted = true
				end
			end
		end
		
		if hasBodies and not looted then
			player:sendTextMessage(MESSAGE_LOOT, config.messageErrorOwner)
			return
		end
	else
		-- shift + right click inside corpse window
		-- this way of looting does not show amount of corpses looted
		if bit.band(position.y, 0x40) ~= 0 then
			local corpse = player:getContainerById(position.y - 0x40)
			if not corpse or corpse and not corpse:isCorpse() then
				return
			end
			
			local corpseTile = Tile(corpse:getPosition())
			if corpseTile and corpseTile:getHouse() then
				-- no looting inside houses
				return
			end
		
			local owner = corpse:getCorpseOwner()
			if owner ~= player:getId() and owner ~= 0 then
				player:sendTextMessage(MESSAGE_LOOT, config.messageErrorOwner)
				return
			end
			
			lootedItems, lootedGold, itemCount, goldCount = internalLootCorpse(player, corpse, lootedItems, lootedGold)
		end
	end
	
	-- response
	player:sendTextMessage(MESSAGE_LOOT, getLootResponse(lootedItems, lootedGold, itemCount, goldCount, corpseCount))
end
setPacketEvent(AUTOLOOT_REQUEST_QUICKLOOT, parseRequestQuickLoot)

-- auto loot update
function parseRequestUpdateAutoloot(player, recvbyte, msg)		
	player:setStorageValue(PlayerStorageKeys.autoLootMode, msg:getByte())	
	local listSize = math.min(msg:getU16(), config.maxLootListLength)
	local lootList = {}
	
	for listIndex = 1, listSize do
		lootList[#lootList + 1] = msg:getU16()
	end
	AutoLoot[player:getId()] = lootList
end
setPacketEvent(AUTOLOOT_REQUEST_SETSETTINGS, parseRequestUpdateAutoloot)

-- login
do
	local creatureEvent = CreatureEvent("AutoLootLogin")
	function creatureEvent.onLogin(player)
		player:registerEvent("AutoLootLogout")
		return true
	end
	creatureEvent:register()
end

-- logout
do
	local creatureEvent = CreatureEvent("AutoLootLogout")
	function creatureEvent.onLogout(player)
		AutoLoot[player:getId()] = nil
		return true
	end
	creatureEvent:register()
end