# WinMute

<div align="center"><img alt="WinMute's logo" title="WinMute" src="WinMute/icons/app.png"></div>


**WinMute** is a small and simple tool, that automatically mutes (and unmutes) your workstation
based on triggers, e.g. the screensaver starts, or your bluetooth headphone disconnects.

It was created to not repeatedly annoy my co-workers with random sounds or music from my
computer whenever I left the room and forgot to mute all sounds.

## Features

* WinMute can automatically mute all sound devices on your workstation when:
  * you lock your workstation.
  * the screensaver turns on.
  * the display turns off.
  * you log off or switch user.
  * your workstation shuts down, goes into hibernate or goes to sleep.
  * your bluetooth headset/headphones disconnect.
  * your workstation is connected to a particular wireless network.
    * alternatively: your Workstation is _not_ connected to particular wireless network
* WinMute is small and only needs a few kilobytes disk space
* WinMute is ad-free, telemetry-free and generally does not send any data whatsoever


## Screenshots

![Screenshot of WinMute](Dist/screenshots/app.png? "Screenshot of WinMute")
![Screenshot of the Settings](Dist/screenshots/settings.gif? "Settings dialog")


## Development

Check out [CONTRIBUTING.md](CONTRIBUTING.md), if you are interested in participating.


## Usage

### Requirements

* Windows Vista or any newer version of Windows.
* [Visual Studio 2015-2022 Redistributable](https://support.microsoft.com/help/2977003/the-latest-supported-visual-c-downloads)

### Installation

Either unzip it to your favourite directory or run the WinMute-Setup.msi and you are all set up!

### Uninstalling WinMute

Just delete the WinMute-directory (or uninstall it from Windows' Apps control panel),

If you want to also remove your personal WinMute settings, open the registry via `regedit.exe` and delete the Folder located in `HKEY_CURRENT_USER\Software\lx-systems\WinMute`.

*Note:* It's possible that you cannot remove the `ScreensaverNotify*.dll` file right away. If you get an error trying to remove it, just wait until your next reboot and delete it afterwards. This is unfortunately a windows limitation.

### How to (un)mute

Just start it and you are good to go!

Whenever you lock your screen from now on or the screensaver starts, WinMute will automatically mute your windows volume, and unmute it right away when you come back to your pc.
If you want to change the behaviour or explore all the other options, right-click on the taskbar notification icon and explore!
