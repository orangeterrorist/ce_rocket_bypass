> # **Rocket Bypass**
I used this to bypass EAC's checks for cheat engine at the time, using [KDU](https://github.com/hfiref0x/KDU) to toggle dse (driver signature enforcement) during runtime. Patchguard will bluescreen your pc if you don't use [EfiGuard](https://github.com/Mattiwatti/EfiGuard).

To use, run `AutoLaunch.bat` or follow the manual instructions.

> **Manual Instructions**

- 1 ) Run this command in your cmd but replace it with the path of your kdu
  - ```C:\PATH\Rocket_Bypass\KDU-1.2.6\Bin\kdu.exe -dse 0```
- 2 ) Open `lunarengine-x86_64.exe` from the `Lunar-Engine` folder
- 3 ) In lunar engine's settings enable:
  - `Use kernelmode debugger` & `Ability to step through kernel code` ( in Debugger Options)
  - `Read/Write Process Memory (Will cause slower scans)` (in Extras)
- 4 ) Run this command in your cmd but replace it with the path of your kdu
  - ```C:\PATH\Rocket_Bypass\KDU-1.2.6\Bin\kdu.exe -dse 6```
