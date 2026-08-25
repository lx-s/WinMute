# Changelog

## 2.6.0.0 - 2026-08-22

### Added

 - Configurable global hotkey that toggles muting of all managed
   audio endpoints.
 - QuietHours are now configurable multiple time slots per day.
 - Mute on Laptop-Lid close
 - Pause music
 - New translations
 - Program is now build with control-flow enforcement (CET)
 - Overhauled settings UI (it was getting a bit crowded in there)

### Fixed

 - Display stand-by detection
 - 60 second cadence time to mute/unmute devices that take a few
    seconds to wake up and register as "plugged in" again.
 - Possible race conditions
 - Memory leak in wifi detector
 - Hardened update checker
 - No longer immediately exiting if the workstation-lock notification
   can not be registered. WinMute will retry registering with TermService
   for about half a minute and disable mute-on-lock if the service does not
   respond until then.
 - Chocolatey uninstall now removes all settings.

## 2.5.5 - 2026-01-23

### Added

 - Try pausing/resuming currently playing media when muting/unmuting.

### Fixed

 - Loading of languages that have certain translations missing.
 - Rare crashes when trying to get the audio device name from the system.

## 2.5.4 - 2025-11-12

### Fixed

 - "Save" Button not enabling, when adding a new WiFi SSID

## 2.5.3 - 2025-10-24

### Added

 * Korean translation (thanks to https://translate.codeberg.org/user/jseo/)
 * Latvian translation (thanks to https://github.com/Coool)
 * Russian translation (thanks to https://translate.codeberg.org/user/yurtpage/)
 * New logging Dialog, so the protocol can be viewed easily without having to store it to disk

## 2.5.2 (2025-02-17)

### Added

 - French translation (thanks to https://translate.codeberg.org/user/avalondrey/)

### Fixed

 - Dialog when adding new devices to mute list (#38)
 - Uninstall icon in add/remove programs control panel

## 2.5.1 - 2024-05-03

### Added
 - Simplified Chinese translation (thanks to https://translate.codeberg.org/user/richzhl/)
 - Updated translations (thanks to all contributors!)

## 2.5.0.1 - 2024-02-26

### Fixed

 - Fixed update-check file for chocolatey package manager

## 2.5.0 - 2024-02-06

### Added
 - WinMute now supports multiple languages!
   If your language is missing, and you want to contribute, please check
   the WinMute GitHub project.
   Thanks goes out for @bovirus for suggesting this feature, helping out with
   the italian language version and providing help and feedback throughout
   the implementation.
 - The following languages are available:
   - German
   - English
   - Spanish
   - Italian
   - Dutch (partially)
 - Optional update notification (disabled for installations via package manager)

### Fixed

 - Some small bugfixes

## 2.4.1 - 2023-09-29

### Fixed

 - Fixes crash on signout and shutdown

## 2.4.0 - 2023-09-22

### Added
 - Option to delay muting by a configurable amount of seconds.

## 2.3.1 - 2023-07-27

### Fixed
 - Annoying error message when the Windows registry key "SystemUsesLightTheme" is not present.

## 2.3.0 - 2023-04-25

### Added

 - Only mute specific endpoints (as is good tradition, also added a switch to NOT mute specific endpoints)

### Changed

- Use `SetTimer` insted of `SetCoalescableTimer` so WinMute can run under Windows 7.
  Please note that Windows 7 is nevertheless not supported any more.

### Fixed

 - WinMute never unmutes, when the PC bootet during quiet hours.
 - Bluetooth muting description

## 2.2.0 - 2022-11-28

### Removed

 - Removed screensaver detection as it caused problems with anti-cheat software
   of current games (e.g. Overwatch 2, Darktide).

## 2.1.2 - 2022-08-25

### Changed
 - Bluetooth detection logic: Muting the workstation doesn't happen,
   when scanning for new devices.
 - Fixed crash when connectiong via remote desktop
 - Show log-file path in settings UI
 - Enhanced logging

## 2.1.1 (2022-08-03)

### Added

 - Added a 5 second delay before unmuting the workstation,
   when a bluetooth device reconnects. This should prevent
   music blasting out of your pc, when the device is connected,
   but the audio endpoint has not been changed yet.

### Changed

 - Improved UI when WLAN or Bluetooth is not available
 - Integrated Bluetooth muting into overall mute logic, so that
   the workstation is not unmuted, when it is locked but an audio
   Bluetooth device reconnects.

## 2.1.0 - 2022-08-02

### Added

 - New feature to mute workstation when an bluetooth audio device
   disconnects (and re-enable audio if it reconnects)

### Fixed

 - Fixed saving of SSID-lists
 - Fixed some spelling errors

## 2.0.0 - 2022-05-01

### Added

 - WinMute now mutes all audio endpoints and not just the default endpoint.
   It also stores all endpoint states and restores them if the appropriate
   option is set.
 - WinMute can now mute your computer when it is connected (alternatively
   when it is _not_ connected) to a particular wireless network.
 - Detection for display standby
 - Detection for RDP sessions
 - Autostart option (no more fiddling with the Startup-folder)
 - Hi-DPI awareness
 - Tray icon for bright system theme

### Changed

 - Reworked mute detection logic. This should fix timing errors, e.g. when
   screensaver with screenlock is active.
 - UI Rework (New settings and about dialogue)
 - Updated project to VS 2022

### Fixed

 - Various fixes and improvements

## 1.6.0 - 2020-12-18

### Added

 - "Quiet Hours": A time frame where WinMute automatically mutes and afterwards
   unmutes your workstation.
 - Added proxy process to recognize screensaver startup when the current
   foreground application is a 32-bit application

### Removed

 - 32-bit build.

## 1.5.0 - 2020-06-29

### Added

 - Mute on suspend, shutdown and logout.
   Automatic audio-restore is disabled for these events.

## 1.4.6 - 2020-04-24

### Added

 * "Support"-Link to About dialog

### Changed

 - Compiled with spectre migitations... just 'cause
 - Small menu redesign.

## 1.4.5 (2019-09-04)

### Changed

 - Upgraded compiler toolset to VS2019

### Fixed

 - Crash when all audio endpoints are removed while the program is running
   (might happen, when connecting to your PC via RDP).

## 1.4.4 - 2017-08-14

### Added

 - New, simpler icon
 - System notification area icon is now white, to fit better with with windows'
   default icons

### Changed

 - Upgraded compiler toolset to VS2017

## 1.4.3 - 2016-08-11

### Changed

 - Upgraded to new compiler toolset
 - Use more modern UI elements

### Removed

 - Removed Windows XP as I can also no longer test it.

## 1.4.1/1.4.2 - 2014-07-17

### Fixed

 * Screensaver muting: It sometimes happened that WinMute
   muted Windows Audio for no apparent reason and "forgot" to
   unmute. This is now fixed.

## 1.4 (2014-07-10)

### Added

 - WinMute can now also mute if the screensaver starts due to windows
   restrictions, unmuting the audio can take up to one second after the
   screensaver resumes.
 - Toggle switch to configure if WinMute should unmute at all after
   a workstation lock or a screensaver run

## 1.3 (2012-02-23)

### Fixed

 - Right-click menu is now correctly dismissed if the user clicks somewhere
   outside of the menu.
 - Vista/7/8: Correctly mute the new device, if the default audio endpoint
   changes.
 - XP: Greatly improved hardware detection code. WinMute now mutes all devices,
   instead of just setting the volume of the first device it finds to zero.
   If the Mute-Button is locked via Group Policies, WinMute will try to
   use the Volume Slider to "emulate" sound muting.

### Changed

 - Embedded Visual C++ 2010 DLLs. This increases file size by about 100kb, but
   enables the usage of this tool in workplaces that do not have deployed the
   Visual C++ 2010 Runtime Environment yet.
 - XP: Actively tries to prevent other programs from unmuting the system.

## 1.2 - 2011-05-30

### Fixed

 - WinMute could malfunction if the audio device was changed or
   disconnected during a windows session.

## 0.0 - 1.1

### Added

 * Everything. Unfortunately I did not keep changelogs.
