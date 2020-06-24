simDR_flap_ratio_control        = find_dataref("sim/cockpit2/controls/flap_ratio")
simDR_parking_brake_ratio       = find_dataref("sim/cockpit2/controls/parking_brake_ratio")
function B747_speedbrake_lever_DRhandler()
  print("speedbrake changed "..simDR_flap_ratio_control)
  simDR_parking_brake_ratio=B747DR_speedbrake_lever/2000
end
B747DR_speedbrake_lever     	= create_dataref("laminar/B747/flt_ctrls/speedbrake_lever", "number", B747_speedbrake_lever_DRhandler)
