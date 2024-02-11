# WinMute
<!-- badges --> 
<div align="right">
  <a href="https://translate.codeberg.org/engage/winmute/">
    <img src="https://translate.codeberg.org/widget/winmute/winmute/svg-badge.svg" alt="Translation status" />
  </a>
</div>

<div align="center">
  <img alt="WinMute's logo" title="WinMute" src="WinMute/icons/app.svg" style="height:128px; margin-bottom: 20px">
</div>

**WinMute** is a small and simple tool, that automatically mutes (and unmutes) your workstation
based on triggers, e.g. the screensaver starts, or your bluetooth headphone disconnects.

It was created to not repeatedly annoy my co-workers with random sounds or music from my
computer whenever I left the room and forgot to mute my computer.

## Features

* WinMute can automatically mute all sound devices on your workstation when:
  * you lock your workstation.
  * the display turns off.
  * you log off or switch user.
  * your workstation shuts down, goes into hibernate or goes to sleep.
  * your bluetooth headset/headphones disconnect.
  * your workstation is connected to a particular wireless network.
    * alternatively: your Workstation is _not_ connected to particular wireless network
* WinMute is small and only needs a few kilobytes of disk space
* WinMute is ad-free, telemetry-free and generally does not send any data whatsoever

## Screenshots

![Screenshot of WinMute](Dist/screenshots/app.png? "Screenshot of WinMute")
![Screenshot of the Settings](Dist/screenshots/settings.gif? "Settings dialog")

## Languages

WinMute is currently available in the following languages:

[![Translation Status](https://translate.codeberg.org/widget/winmute/winmute/multi-auto.svg)](https://translate.codeberg.org/engage/winmute/)

## Contributions

If you are fluent in another language, want to fix some bugs or pitch some new ideas,
please take a look at [CONTRIBUTING.md](CONTRIBUTING.md).

## Usage

### Requirements

* Windows 7 or any newer version of Windows.
* [Visual Studio 2015-2022 Redistributable](https://support.microsoft.com/help/2977003/the-latest-supported-visual-c-downloads)

### Installation

Either unzip it to your favourite directory or run the Setup.exe and you are all set up!

### Uninstalling WinMute

If you've installed it with the setup, uninstall it from the Windows programs control panel).
If you've installed it without the setup, just delete it.

If you want to also remove your personal WinMute settings, open the registry via `regedit.exe` and delete the Folder located in `HKEY_CURRENT_USER\Software\lx-systems\WinMute`.

### How to (un)mute

Just start it and you are good to go!

Whenever you lock your screen from now on or the screensaver starts, WinMute will automatically mute your windows volume, and unmute it right away when you come back to your pc.
If you want to change the behaviour or explore all the other options, right-click on the taskbar notification icon and explore!
