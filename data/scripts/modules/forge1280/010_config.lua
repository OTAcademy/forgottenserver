do
	-- this table has three functions:
	-- number of classes in game
	-- price for each class/tier
	-- number of tiers in classes (autogenerated from prices)
	-- NOTE: 1kk = 100cc
	
	--
	-- BEGIN CONFIG
	--
	local forgeMeta = {
		[1] = { -- class
			25000 -- price to fuse two [class 1, tier 0] items into one tier 1 item
		},
		[2] = {
			750000, -- 750k
			5 * STACK_100CC -- 5kk
		},
		[3] = {
			4 * STACK_100CC, -- 4kk
			10 * STACK_100CC, -- 10kk
			20 * STACK_100CC -- 20kk
		},
		[4] = {
			8 * STACK_100CC, -- 8kk
			20 * STACK_100CC, -- 20kk
			40 * STACK_100CC, -- 40kk
			65 * STACK_100CC, -- 65kk
			100 * STACK_100CC, -- 100kk
			250 * STACK_100CC, -- 250kk
			750 * STACK_100CC, -- 750kk
			2500 * STACK_100CC, -- 2500kk
			8000 * STACK_100CC, -- 8000kk
			15000 * STACK_100CC -- 15000kk
		}
	}

	local forgeData = {
		-- server side forge logic
		allowCustomItemFusion = false, -- original: false
		allowCustomItemTransfer = false, -- original: false
		historyEntriesPerPage = 9,
		
		-- bytes sent to the client
		sliversDustCost = 20, -- (conversion) (left column top) cost to make 1 bottom item
		sliversPerConversion = 3, -- (conversion) (left column bottom) how many items to make
		coreSliversCost = 50, -- (conversion) (middle column top) cost to make 1
		dustLimitIncreaseCost = 75, -- (conversion) (right column top) current stored dust limit minus this number = cost to increase stored dust limit
		minStoredDustLimit = 100, -- (conversion) (right column bottom) starting stored dust limit
		maxStoredDustLimit = 225, -- (conversion) (right column bottom) max stored dust limit
		fusionDustCost = 100, -- (fusion) dust cost
		transferDustCost = 100, -- (transfer) dust cost
		fusionBaseSuccessRate = 50, -- (fusion) base success rate
		fusionBonusSuccessRate = 15, -- (fusion) bonus success rate
		fusionReducedTierLossChance = 50 -- (fusion) tier loss chance after reduction
	}
	
	local forgeBonuses = {
		-- NOTE: actual rates are unknown and are difficult to measure due to the prices
		-- bonuses possible to roll on fusion success
		-- [bonus type] = chance points
		-- percent chance will be calculated based on sum of all entries
		-- (if chance points 1 and sum of all is 10, one point will be 10% chance)
		-- default config counts 100 as 1%
		[FORGE_BONUS_NONE] = 7500,
		[FORGE_BONUS_DUSTKEPT] = 500, -- dust not consumed
		[FORGE_BONUS_CORESKEPT] = 500, -- cores not consumed
		[FORGE_BONUS_GOLDKEPT] = 500, -- gold not consumed
		[FORGE_BONUS_ITEMKEPT_ONETIERLOST] = 250, -- item not consumed, lost 1 tier only
		[FORGE_BONUS_ITEMKEPT_NOTIERLOST] = 250, -- second item and its tier kept
		[FORGE_BONUS_BOTHUPGRADED] = 250, -- both items gained tier
		[FORGE_BONUS_EXTRATIER] = 250, -- item gained two tiers
		[FORGE_BONUS_ITEMNOTCONSUMED] = 0, -- item not consumed
	}	
	--
	-- END CONFIG
	--
	
	-- register fusion total chance
	local totalChance = 0
	for _, chance in pairs(forgeBonuses) do
		totalChance = totalChance + chance
	end
	forgeBonuses.totalChance = totalChance
	
	-- getters for the tables
	function getForgeMeta()
		return forgeMeta
	end
	function getForgeData()
		return forgeData
	end
	function getForgeBonuses()
		return forgeBonuses
	end
end