ias_kts_pilot = find_dataref("sim/cockpit2/gauges/indicators/airspeed_kts_pilot")
xtlua_async = find_dataref("laminar/xtlua/xtlua_async")
xtlua_async_array = find_dataref("laminar/xtlua/xtlua_async_array")
xtlua_async_string      = find_dataref("laminar/xtlua/xtlua_async_string")
xtlua_async_string = "hello"
simCMD_com1_stby_flip       	       = find_command("sim/radios/com1_standy_flip")

function simCMD_com1_stby_flip_wrapper_beforeCMDhandler(phase, duration)
  print("before com1 flip "..phase.." for "..duration)
end
function simCMD_com1_stby_flip_wrapper_afterCMDhandler(phase, duration)
  print("after com1 flip")
end
simCMD_com1_stby_flip_wrapper          = wrap_command("sim/radios/com1_standy_flip", simCMD_com1_stby_flip_wrapper_beforeCMDhandler, simCMD_com1_stby_flip_wrapper_afterCMDhandler)
local ref
function after_physics()
  print("during physics 2")
  ref=xtlua_async+2;
  xtlua_async_string = "lua" .. ref
  local ref2=ias_kts_pilot*ref
  --print(ref2)
  simCMD_com1_stby_flip:once()
  xtlua_async_array[2]=ias_kts_pilot*ref
  if ref>1000 then ref=0 end
  --print(ias_kts_pilot)
  --print(xtlua_async)
  xtlua_async=ref;
  
  
  --for y=0,200000 do print(y) end
  
end 

