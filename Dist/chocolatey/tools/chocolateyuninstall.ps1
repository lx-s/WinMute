Stop-Process -Name WinMute -ErrorAction SilentlyContinue
Remove-Item "HKCU:\SOFTWARE\lx-systems\WinMute" -Recurse -ErrorAction SilentlyContinue
Remove-ItemProperty -Path "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" -Name "LX-Systems WinMute" -ErrorAction SilentlyContinue
