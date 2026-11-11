# CreoOS Windows flash + listen toolkit
Place it wherever (e.g. C:\Users\szymt\Documents\CreoOS-boot).

## 1) Flash via Odin (AP slot)
Use `CreoOS-boot.tar.md5` from this folder:
1. Odin 3.14.4 (Windows)
2. phone in **Download Mode** (adb reboot download, accept with VolUp)
3. **AP** button -> select `CreoOS-boot.tar.md5`
4. Options: leave defaults (Auto Reboot + F.Reset Time ok)
5. Start. If flashing fails: re-download cable, retry Download Mode.

Safety: only the `boot` partition is touched. Restore = flash your
original `boot.img` backup the same way.

## 2) UART listen (PowerShell)
```
powershell -ExecutionPolicy Bypass -File .\listen.ps1
```
Logs land in `CreoOS-boot\serial.log` — that's the file to send back over
USB or WSL. Requires a **USB-C UART jig (510ohm)** for physical phones —
WITHOUT the jig the S10's UART lines aren't wired to USB-C by default.

Fallback captures (non-jig):
```
adb logcat -d     > .\adb.log
adb shell dmesg   > .\dmesg.log
```

## 3) Return the proof
Copy `serial.log` / `adb.log` into `\\wsl$\Ubuntu\home\szymt\CreoOS\out\`
(or just plug via USB and `adb pull`) — the AI reads it out of there.

## 4) Output path (drop the log here)
After flashing, drop the serial.log/adbdump in:
  \\wsl$\Ubuntu\home\szymt\CreoOS\out\
(from file explorer or: copy serial.log "\\wsl$\Ubuntu\home\szymt\CreoOS\out\")
