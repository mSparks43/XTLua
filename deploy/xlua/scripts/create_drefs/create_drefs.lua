--[[
*****************************************************************************************


*****************************************************************************************
*        COPYRIGHT � 2019 Mark Paker- ALL RIGHTS RESERVED	    *
*****************************************************************************************
--]]


--*************************************************************************************--
--** 				                X-PLANE DATAREFS            			    	 **--
--*************************************************************************************--
function null_command(phase, duration)
end
function deferred_command(name,desc)
	c = XLuaCreateCommand(name,desc)
	print("Deffereed "..name)
	XLuaReplaceCommand(c,null_command)
	return make_command_obj(c)
end
xtlua_async      = XLuaCreateDataRef("laminar/xtlua/xtlua_async", "number","yes",nil)
xtlua_async_array      = XLuaCreateDataRef("laminar/xtlua/xtlua_async_array", "array[4]","yes",nil)
xtlua_async_string      = XLuaCreateDataRef("laminar/xtlua/xtlua_async_string", "string","yes",nil)
B747CMD_fms1_ls_key_L1              = deferred_command("laminar/B747/fms1/ls_key/L1", "FMS1 Line Select Key 1-Left")



