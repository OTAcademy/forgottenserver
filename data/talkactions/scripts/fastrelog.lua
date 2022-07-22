function onSay(player, words, param)
	local relog = player:fastRelog(param)
	
	if relog == RETURNVALUE_YOUCANNOTLOGOUTHERE then
		player:sendColorMessage("You may not logout here!", MESSAGE_COLOR_PURPLE)
	elseif relog == RETURNVALUE_NOTPOSSIBLE then
		player:sendColorMessage("Unable to login your character. Make sure you spelled your name correctly.", MESSAGE_COLOR_PURPLE)
	elseif relog == RETURNVALUE_YOUMAYNOTLOGOUTDURINGAFIGHT then
		player:sendColorMessage("You may not logout during or immediately after a fight!", MESSAGE_COLOR_PURPLE)
	end
	
	return false
end
