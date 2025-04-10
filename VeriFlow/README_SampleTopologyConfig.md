S#  // Switch Definitions Section
<SwitchID>:<NextHop1>,<NextHop2>,...
<SwitchID>:<NextHop1>,<NextHop2>,...

Example -> 10.0.0.1:10.0.0.2,10.0.0.3
		   10.0.0.2:
		   10.0.0.3:10.0.0.1

Hosts are not considered as a "next hop". Only switches.

H#  // Host Definitions Section
<HostID>:<SwitchID>

Example -> 10.0.0.31:10.0.0.1

R#  // Rules Definitions Section
<SwitchID>-<Prefix>-<NextHopId>

Example -> 10.0.0.1-192.168.1.0/24-10.0.0.2
This rule matches all traffic under the 192.168.1.0/24 subnet, and forwards it to 10.0.0.2
For most rules, drop is essentially the default action -- everything added here is considered as a forward.

E!  // End Marker. Use this at end of file, after rules section.