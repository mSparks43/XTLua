ias_kts_pilot = find_dataref("sim/cockpit2/gauges/indicators/airspeed_kts_pilot")
xtlua_async = find_dataref("laminar/xtlua/xtlua_async")
xtlua_async_array = find_dataref("laminar/xtlua/xtlua_async_array")
xtlua_async_string      = find_dataref("laminar/xtlua/xtlua_async_string")
xtlua_async_string = "hello"
local ref
function after_physics()
  print("during physics 2")
  ref=xtlua_async+2;
  xtlua_async_string = "lua" .. ref
  local ref2=ias_kts_pilot*ref
  print(ref2)
  xtlua_async_array[2]=ias_kts_pilot*ref
  if ref>1000 then ref=0 end
  print(ias_kts_pilot)
  print(xtlua_async)
  xtlua_async=ref;
  
  
  --for y=0,200000 do print(y) end
  
end 

