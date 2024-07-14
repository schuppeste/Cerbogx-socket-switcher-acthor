# Cerbogx-socket-switcher-acthor for ESP32

The ESP32 reads the Battery Voltage,SOC,DC-Current and Multiplus Load by TCP Modbus on CerboGX PV Control Device and controls Fritz Dect200 Power Sockets/All http Power Sockets by own Configuration.

New: My-PV ac-thor Control on BatterySoc+Battery Current, Voltage, Limit to maximum Load, Control Algorithm und several Settings.

If all is good, the ESP32 increase the Acthor Power in defined Steps, otherwise itdecreases the Power.
Show scrrenshot for Details.

Ac-Thor Settings:

![Screenshot 2024-07-13 at 21-24-49 Main Configuration](https://github.com/user-attachments/assets/1bcf0ecf-3903-4b89-b82a-b618b348e3f3)


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
