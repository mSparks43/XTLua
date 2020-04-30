ias_kts_pilot = find_dataref("sim/cockpit2/gauges/indicators/airspeed_kts_pilot")
xtlua_async = find_dataref("laminar/xtlua/xtlua_async")
xtlua_async_array = find_dataref("laminar/xtlua/xtlua_async_array")
xtlua_async_string      = find_dataref("laminar/xtlua/xtlua_async_string")
fms1_line0=find_dataref("sim/cockpit2/radios/indicators/fms_cdu1_text_line0")
xtlua_async_string = "hello"
simCMD_com1_stby_flip       	       = find_command("sim/radios/com1_standy_flip")
function deferred_dataref(name,nilType,callFunction)
  if callFunction~=nil then
    print("WARN:" .. name .. " is trying to wrap a function to a dataref -> use xlua")
    end
    return find_dataref(name)
end
function simCMD_com1_stby_flip_wrapper_beforeCMDhandler(phase, duration)
  print("before com1 flip "..phase.." for "..duration)
end
function simCMD_com1_stby_flip_wrapper_afterCMDhandler(phase, duration)
  print("after com1 flip")
end
simCMD_com1_stby_flip_wrapper          = wrap_command("sim/radios/com1_standy_flip", simCMD_com1_stby_flip_wrapper_beforeCMDhandler, simCMD_com1_stby_flip_wrapper_afterCMDhandler)
local ref
function flight_start() 

    print("XTLua flight start")

end
NUM_ALERT_MESSAGES = 5
B747DR_CAS_gen_memo_msg = {}
for i = 0, NUM_ALERT_MESSAGES do
    B747DR_CAS_gen_memo_msg[i] = deferred_dataref(string.format("laminar/B747/CAS/gen_memo_msg_%03d", i), "string")
end

function after_physics()
  print("during physics 4")
  local testXPString=fms1_line0
  print(testXPString)
  B747DR_CAS_gen_memo_msg[0] = "help me"
  --[[ref=xtlua_async+2;
  xtlua_async_string = "lua" .. ref
  local ref2=ias_kts_pilot*ref
  
  --print(ref2)
  simCMD_com1_stby_flip:once()
  xtlua_async_array[2]=ias_kts_pilot*ref
  if ref>1000 then ref=0 end
  --print(ias_kts_pilot)
  --print(xtlua_async)
  xtlua_async=ref;]]
  
  
  --for y=0,200000 do print(y) end
  
end 

