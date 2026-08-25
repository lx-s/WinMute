# WinMute Privacy Policy

**Applies to:** WinMute for Windows, all versions and distribution channels
**Effective date:** 2026-08-25
**Publisher:** Alexander Steinhoefer (lx-systems)
**Contact:** kontakt@lx-s.de

## Summary

WinMute does not collect, transmit, sell, or share any personal data.

It has no accounts, no advertising, no analytics, and no telemetry. Everything
WinMute needs in order to work stays on your device. Its only optional network
feature is a check for new versions, which is switched off unless you turn it on
(see section 5).

## 1. What WinMute does

WinMute is a small utility that automatically mutes and unmutes your Windows
audio devices in response to events on your computer — for example when you lock
your workstation, when the display turns off, when your Bluetooth headphones
disconnect, or when you connect to a particular wireless network.

All of this happens locally, on your machine, using standard Windows APIs.

## 2. Data WinMute stores on your device

WinMute saves your settings so they survive a restart. This data never leaves
your computer, is not encrypted or uploaded anywhere, and is only readable by
your own Windows user account.

The settings include:

* Which mute triggers you enabled (lock, display standby, sleep, shutdown,
  log-off, lid close, remote desktop, and so on).
* The names of Wi-Fi networks (SSIDs) you added to your mute or allow list.
* The names and addresses of Bluetooth devices you added to your device list.
* The names of the audio devices (endpoints) you chose to manage individually.
* Your quiet-hours schedule.
* Interface preferences such as the display language, notification settings, and
  your global mute hotkey.

The settings are stored in the Windows registry under your own user account, in
`HKEY_CURRENT_USER\SOFTWARE\lx-systems\WinMute`. If you installed WinMute from
the Microsoft Store, Windows keeps that data inside the app package's own
private, per-user storage instead.

## 3. Information WinMute reads from your system

To decide when to mute, WinMute reads the following from Windows while it is
running. This information is evaluated in memory, used for the mute decision,
and then discarded. It is never stored beyond what you explicitly configured in
section 2, and never transmitted.

* **Audio devices:** the names and mute/volume state of your playback devices,
  so it can mute and restore them.
* **Wireless networks:** the name (SSID) of the Wi-Fi network you are currently
  connected to, so it can be compared against your own list. WinMute does not
  read your Wi-Fi passwords, saved profiles, or network traffic.
* **Bluetooth:** the state of your Bluetooth radio and the names of connecting
  and disconnecting devices, so it can react to your headphones. WinMute does
  not pair with devices or read their content.
* **Session and power events:** notifications from Windows that the workstation
  was locked, the display switched off, or the system is suspending.
* **Media playback:** if you enable the option, WinMute sends a pause or play
  command to your media applications. It does not read what you are playing.

## 4. Optional log file

Logging is **switched off by default**. If you turn it on in the settings — for
example because you were asked to when reporting a bug — WinMute writes a plain
text log file to your Windows temporary folder (`%TEMP%`).

The log records what WinMute did and why, which can include the names of your
audio devices, Wi-Fi networks, and Bluetooth devices. The file stays on your
computer. WinMute never uploads it.

When you switch logging off again, WinMute deletes the log file. You can also
delete it yourself at any time.

If you choose to attach the log to a public bug report, please review it first —
anything you send us is shared by your own decision, not by the app.

## 5. Network connections and the update check

The only part of WinMute that uses the internet is the optional update check,
and it is **switched off by default**. Unless you tick "Check for updates on
startup" in the settings, WinMute never connects to anything.

If you do enable it, then each time WinMute starts it downloads one small,
static text file over an encrypted HTTPS connection:

```
https://raw.githubusercontent.com/lx-s/WinMute/main/CURRENT_VERSION
```

That file lists the latest available version number. WinMute compares it with
the version you are running and, if yours is older, offers you a link to the
download page. Nothing is uploaded: the request carries no information about
you, your device, your settings, or your usage — not even the version you
currently have installed.

As with any download, the server hosting the file (GitHub, operated by GitHub,
Inc.) necessarily sees your IP address and the time of the request in order to
answer it. That data is handled under
[GitHub's Privacy Statement](https://docs.github.com/site-policy/privacy-policies/github-privacy-statement).
We receive no report of these requests and cannot identify who checked for
updates. If you would rather not contact GitHub at all, simply leave the update
check switched off.

In builds distributed through the Microsoft Store the update check is disabled
altogether and cannot be enabled, because the Store keeps the app up to date
itself.

WinMute contains no other network functionality of any kind.

## 6. Where you got WinMute

This policy covers the WinMute application itself. How you obtained it is a
separate matter: downloading, installing, and updating software through a store
or package manager involves that provider, under their own privacy policy and
not ours. Depending on the channel this may be the
[Microsoft Store](https://privacy.microsoft.com/privacystatement),
[winget](https://privacy.microsoft.com/privacystatement),
[Chocolatey](https://chocolatey.org/privacy), or
[GitHub](https://docs.github.com/site-policy/privacy-policies/github-privacy-statement)
for direct downloads and releases.

From these providers we receive at most anonymous, aggregated figures such as
download or install counts. These do not identify individual users, and we have
no way to link them to a person or a device.

Likewise, if WinMute were to crash, Windows Error Reporting may send diagnostic
information to Microsoft according to your Windows settings. That is a Windows
feature and outside our control.

## 7. Sharing, selling, and third parties

We do not collect personal data, so there is nothing to share. We do not sell,
rent, or transfer data to third parties. WinMute contains no advertising
networks, no analytics SDKs, and no third-party tracking components.

## 8. Data retention and deletion

Your settings remain on your device for as long as you keep them there.

* **Microsoft Store:** uninstalling the app through Windows removes it together
  with its stored settings.
* **Setup or package manager:** uninstall WinMute the usual way. Your personal
  settings are deliberately left behind so they survive a reinstall; to remove
  them as well, delete the key `HKEY_CURRENT_USER\SOFTWARE\lx-systems\WinMute`
  with the Windows registry editor (`regedit.exe`).
* **Portable/ZIP version:** delete the WinMute folder, and the registry key
  above if you want the settings gone too.

To delete the optional log file, either switch logging off in WinMute — which
deletes it immediately — or remove the WinMute log from your `%TEMP%` folder.

## 9. Children

WinMute is a general-purpose utility and is not directed at children. It does
not knowingly collect any information from anyone, regardless of age.

## 10. Your rights (GDPR)

We do not process personal data as defined by the GDPR. We operate no servers
and run no service that WinMute talks to: we have no access to your settings,
your device, or any identifier. Even if you enable the optional update check,
your request goes to GitHub as the file's host and never reaches us. There is
therefore no data held by us that could be exported, corrected, or erased on
request.

You remain in full control of the data described in sections 2 and 4 at all
times, since it exists solely on your own computer.

If you believe otherwise, or have any question about how WinMute handles data,
please contact us at the address below.

## 11. Open source

WinMute is open source and released under the 3-clause BSD licence. Every claim
in this policy can be verified in the source code at
<https://github.com/lx-s/WinMute>.

## 12. Changes to this policy

If this policy changes, the updated version will be published at this address
and the effective date at the top will be revised. Material changes will also be
noted in the app's changelog.

## 13. Contact

Alexander Steinhoefer (lx-systems)
Email: kontakt@lx-s.de
Project: <https://github.com/lx-s/WinMute>
