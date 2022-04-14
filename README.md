# WinMute #

### About ###
WinMute can automatically mute all sound devices on your PC when

* you lock your PC.
* the screensaver turns on.
* the display turns off
* you log off
* your PC shuts down
* your PC goes to sleep
* your PC is connected to a particular wireless network
  * alternatively: your pc is not connected to particular wireless network

This is very helpful e.g. if you leave your desk and don't want to annoy your co-workers with random sounds or music from your computer or don't want music blasting out of your speakers, the next time you power on your pc during a presentation.

### Screenshot ###
![Screenshot of WinMute](https://raw.githubusercontent.com/lx-s/WinMute/master/Dist/screenshot.png? "Screenshot of WinMute")

### Requirements ###
* Windows Vista or any newer version of Windows.
* [Visual Studio 2022 Redistributable](https://support.microsoft.com/help/2977003/the-latest-supported-visual-c-downloads)

### Installation ###
Unzip it to your favourite directory and you are all set up!

### Uninstalling WinMute ###
Just delete the `WinMute.exe` and `ScreensaverNotify.dll` file from your hard drive.

If you want to also remove your personal WinMute settings, open the registry via `regedit.exe` and delete the Folder located in `HKEY_CURRENT_USER\Software\lx-systems\WinMute`.

*Note:* It's possible that you cannot remove the `ScreensaverNotify.dll` file right away. If you get an error trying to remove it, just wait until your next reboot and delete it afterwards. This is unfortunately a windows limitation.

### Usage ###
Just start it and you are good to go!

Whenever you lock your screen from now on or the screensaver starts, WinMute will automatically mute your windows volume, and unmute it right away when you come back to your pc.
If you want to disable this behaviour temporary, right-click on the taskbar notification icon and uncheck the menu item `Mute on Workstationlock` / `Mute on Screensaver start`.
