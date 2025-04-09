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

Example -> Not determine how this works yet. In progress.

E!  // End Marker. Use this at end of file, after rules section.