local config = {
	maxCorpsesLimit = 40, -- how many corpses will be checked
	maxLootListLength = 2000, -- how many items can the player register
	messageErrorPosition = "You cannot loot this position.",
	messageErrorOwner = "You are not the owner.",
	
	lootAbsent = "No loot.",
	lootStart = "You looted",
	
	lootGoldNone = "none of the dropped gold",
	lootGoldSome = "only some of the dropped gold",
	lootGoldAll = "complete %d gold",
	
	lootItemNone = "none of the dropped items",
	lootItemSome = "only some of the dropped items",
	lootItemAll = "all items",
}

-- 143 - loot corpse/tile
AUTOLOOT_REQUEST_QUICKLOOT = 0x8F

-- 145 - add/remove loot item
AUTOLOOT_REQUEST_SETSETTINGS = 0x91

LOOTED_RESOURCE_ABSENT = 0 -- corpse has no items
LOOTED_RESOURCE_NONE = 1 -- failed to loot items
LOOTED_RESOURCE_SOME = 2 -- failed to loot some items
LOOTED_RESOURCE_ALL = 3 -- looted all items

local function getLootedStatus(currentAmount, totalAmount, currentStatus)
	if currentAmount == 0 then
		return LOOTED_RESOURCE_ABSENT
	elseif currentAmount == 0 then
		return LOOTED_RESOURCE_NONE
	elseif currentAmount < totalAmount then
		return LOOTED_RESOURCE_SOME
	end
	
	return LOOTED_RESOURCE_ALL
end

local function getNextLootedStatus(currentStatus, elementStatus)
	if currentStatus == elementStatus or currentStatus == LOOTED_RESOURCE_ABSENT then
		return elementStatus
	elseif currentStatus == LOOTED_RESOURCE_ALL or currentStatus == LOOTED_RESOURCE_NONE and (elementStatus == LOOTED_RESOURCE_ALL or elementStatus == LOOTED_RESOURCE_SOME) then
		return LOOTED_RESOURCE_SOME
	end
	
	return currentStatus
end

function internalLootCorpse(player, corpse, lootedItems, lootedGold)
	if not corpse:isContainer() then
		return LOOTED_RESOURCE_ABSENT, LOOTED_RESOURCE_ABSENT
	end
	
	local corpseItems = 0
	local retrievedItems = 0
	local corpseGold = 0
	local retrievedGold = 0
	
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
			else
				retrievedItems = retrievedItems + 1
			end
		else
			lootedItem:remove()
		end
	end
	
	-- looted items response
	return getLootedStatus(retrievedItems, corpseItems, lootedItems), getLootedStatus(retrievedGold, corpseGold, lootedGold)
end

function parseRequestQuickLoot(player, recvbyte, msg)
	local position = Position(msg:getU16(), msg:getU16(), msg:getByte())	
	
	local stackpos = msg:getByte()
	local spriteId = msg:getU16()
	local containerPos = msg:getByte()
	local isGround = msg:getByte() == 1

	local lootedItems = LOOTED_RESOURCE_ABSENT
	local lootedGold = LOOTED_RESOURCE_ABSENT
	
	if position.x ~= CONTAINER_POSITION then
		-- shift + right click on the floor
	
		-- distance check
		if position:getDistance(player:getPosition()) > 1 then
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, config.messageErrorPosition)
			return
		end

		-- tile check
		local tile = Tile(position)
		if not tile then
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, config.messageErrorPosition)
			return
		end
		
		if tile:getHouse() then
			-- no looting inside houses
			return
		end
		
		local hasBodies = false
		local looted = false
		local itemCount = 0
		local goldCount = 0

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
				
				if lootable then
					local tmpLootedItems = LOOTED_RESOURCE_ABSENT
					local tmpLootedGold = LOOTED_RESOURCE_ABSENT
					
					tmpLootedItems, tmpLootedGold = internalLootCorpse(player, corpse, tmpLootedItems, tmpLootedGold)
					
					lootedItems = getNextLootedStatus(lootedItems, tmpLootedItems)
					lootedGold = getNextLootedStatus(lootedGold, tmpLootedGold)
					
					looted = true
				end
			end
		end
		
		if hasBodies and not looted then
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, config.messageErrorOwner)
			return
		end
	else
		-- shift + right click inside corpse window
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
				player:sendTextMessage(MESSAGE_EVENT_ADVANCE, config.messageErrorOwner)
				return
			end
			
			lootedItems, lootedGold = internalLootCorpse(player, corpse, lootedItems, lootedGold)
		end
	end
	
	-- response
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, string.format("loot item response: %d, loot gold response: %d", lootedItems, lootedGold))
end
setPacketEvent(AUTOLOOT_REQUEST_QUICKLOOT, parseRequestQuickLoot)

function parseRequestUpdateAutoloot(player, recvbyte, msg)
	-- u8 mode
	-- u16 list size
		-- list member:
		-- u16 clientId
end
setPacketEvent(AUTOLOOT_REQUEST_SETSETTINGS, parseRequestUpdateAutoloot)