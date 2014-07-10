# WinMute #

### About ###
WinMute automatically mutes your PC volume if you lock your screen or the screensaver is running.
This is very helpful e.g. if you leave your desk and don't want to annoy your co-workers with random sounds or music from your computer.

### Requirements ###
Windows XP SP3 or newer.

### Installation ###
Unzip it to your favourite directory and you are all set up!

If you want WinMute to automatically start with your windows, just create a shortcut to the winmute.exe file in the "Startup" folder located in your Windows Startmenu (to open this folder, press Win+R and enter "Shell:Startup", then click "OK").

### Uninstalling WinMute ###
Just delete the WinMute.exe and ScreensaverNotify.dll`*` file from your hard drive.

If you want to also remove your personal WinMute settings, open the registry via `regedit.exe` and delete the Folder located in `HKEY_CURRENT_USER\Software\lx-systems\WinMute`.

 * It's possible that you cannot remove this DLL file right away. If you get an error trying to remove it, just wait until your next reboot and delete it afterwards. This is unfortunately a windows limitation.


### Usage ###
Start it and that's it!

Whenever you lock your screen from now on or the screensaver starts, WinMute will automatically mute your windows volume, and unmute it right away when you come back to your pc.

If you want to disable this behaviour temporary, right-click on the taskbar notification icon and uncheck the menu item
`Mute on Workstationlock` / `Mute on Screensaver start`.
