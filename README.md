# Cerbogx-socket-switcher-acthor
Reads Battery Voltage on TCP Modbus CerboGX Devices and controls Fritz Dect200 Power Sockets from Configuration.

New: acthor Control on BatterySoc+Battery Current>0 Voltage, Limit to maximum Load, Control Algorithm und several Settings.

Hardcoded: BAttery SOC > 95% & Current>0A & All 3 Phases on Output not in Limit(2,5KW Multiplus)
lastbattsoc > 95 && lastbattcurrent > 0 && lastl1powermax < 2500 && lastl2powermax < 2500 && lastl3powermax < 2500

If all is good, the Modbus Client Reads every "Blocktime-Setting-Time" the Registers on the CerboGX, calling acthor_heartbeat Function after reading.

If the Curennt > 0 for "a_r_i" Times in Heartbeat, added the Value a_r to the Output Power (Reset a_r_i on not okay in 1 measurement, or adding the Value to Power Output)

If the Current < 0 for "a_r_d" (needs Power from Battery),  subtract the Value a_r from the Output Power (Reset a_r_d on not okay in 1 measurement, or adding the Value to Power Output)






Ac-Thor Settings:
![Bildschirmfoto vom 2024-06-10 11-43-06](https://github.com/schuppeste/Cerbogx-socket-switcher-acthor/assets/3218517/1f0e59e5-2ab3-48ef-a745-f78928524470)


Webinterface:

![Bildschirmfoto vom 2024-03-09 14-26-08](https://github.com/schuppeste/Cerbogx-socket-switcher/assets/3218517/036e7964-a692-43dc-98a9-82bef9a9885e)


Configuration:  
Active: Enable or Disable Device  
Min: Voltage to switch off Switch (Hystherese)  
Max: Voltage to switch on Switch (Hystherese)  
Invert: OnOff: switch off at max Voltage, Switch on at min Voltage... OffOn switch, Off at min Voltage, switch on at max Voltage  
delon: On Delay in Minutes  
deloff: Off Delay in Minutes
depon: Needed On  (id (Pos Number in List)0-9) Change state only if Device Id is on
depoff: Needed Off  (id (Pos Number in List)) Change state only if Device Id is off
ontime: switch on Time  
offtime: switch off Time  
itime: change on/off in Between or not Between 

WARRANTY
THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
