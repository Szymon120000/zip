# Flashing CreoOS to beyond2lte (Samsung Galaxy S10+ Exynos)

## PATH = "sboot enforces 'only official released binaries' on BOOT — but the RECOVERY
partition is flashed with samloader-rs and accepts custom images (proven by
LineageOS/Kali flows). Our kernel boots identically from recovery (same load addr
0x10008000). Booting it = VolUp + Bixby + Power. Normal Android boot untouched.

PATH-STEP-HEADING — PREREQS — READ FULLY BEFORE FLASHING (original section below)
1. **Downgrade risk zero, but boot partition is overwritten.** Flashing replaces
   ONLY the `boot` partition — your data (userdata) stays intact. Restore is a
   2-minute `heimdall` flash of the backup.
2. **OEM unlock must be ON and bootloader unlocked** (Samsung Developer menu,
   `adb reboot download`), or sboot REFUSES unsigned boot images. If you have
   never unlocked, stop here and do that first.
3. **Make the backup FIRST** (`./scripts/flash.sh backup`) before any flash —
   this is your restore path if anything goes sideways.

## THE FLOW
```
# on the PC, once:
sudo apt install heimdall-flash adb

# 0) backup existing boot (SAFETY NET)
./scripts/flash.sh backup

# 1) build the phone kernel + boot image
make phone

# 2) put phone in Download Mode: reboot normally, then hold VolumeDown
adb reboot download          # then confirm with VolumeUp when asked

# 3) flash!
./scripts/flash.sh flash

# 4) what you should see
# the phone reboots; sboot loads CreoOS into DRAM at 0x10008000 and jumps.
# if the kernel found a working UART you'll see the CreoOS banner on serial.
# Otherwise: quiet boot, screen off — this is EXPECTED for bring-up stage 1;
# the kernel is alive on DRAM, waiting for the next bring-up pass.
```

## UNBRICK
If the phone ever fails to boot normally:
- power off, hold Vol- to enter Download Mode, then
  `./scripts/flash.sh restore` (your backup is in `backup/`)
- or reflash the stock/LineageOS image you made in step 0.

## WHAT IS INCLUDED TODAY
- fully custom kernel (zero Linux) written for aarch64:
  - own vectors/context switching, scheduler, tiny kprintf, UI skeleton
- QEMU-tested end-to-end (`make run`)
- phone variant with Exynos9820 UART probing (defensive, never hangs)
- Android boot image v1 in sboot's exact expected shape:
  kernel_addr 0x10008000, page 2048, header 1648 bytes — platform-specific
  constants pulled from the LineageOS exynos9820-common config.

## NEXT SESSIONS (bringup ladder)
1. serial output confirmation on the real device (UART jig or USB serial)
2. GICv3 init for the phone's timer (QEMU needed v2, phone is v3)
3. DPU framebuffer (CreoUI on glass — the money shot)
4. Kr touchscreen input, then the WiFi stack plan in README.md

## 2026-11-10: RECOVERY-SLOT PATH (schema from real LOS recovery)
Los recovery.img (LOS 23.2, 20261107) parsed:
  ANDROID! hv1, ksz=47765528 @0x10008000, rsz=15016921 @0x11000000,
  tags 0x10000100, page 2048, recovery_dtbo 1751040, hdr 1648.
Plan: samloader flash -p RECOVERY (los control) -> (-p RECOVERY creo) -> VolUp+Bixby+Power.
