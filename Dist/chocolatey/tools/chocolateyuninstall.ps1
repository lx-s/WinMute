Stop-Process -Name WinMute
Remove-Item "HKCU:\SOFTWARE\lx-systems\WinMute"
Remove-ItemProperty -Path "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" -Name "LX-Systems WinMute"
