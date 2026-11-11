#!/usr/bin/env bash
# CreoOS — RECOVERY-slot flashing suite for beyond2lte (samloader-rs).
# Never touches BOOT/BOOTLOADER/userdata. Stock recovery is restorable.
#
# usage: ./scripts/flash-recovery.sh <command>
#   fixusb    add udev rules so the USB device is writable (run once)
#   los       flash LineageOS recovery (do NOT boot it; control test A)
#   creo      flash our custom kernel image (test B)
#   restore   flash a saved backup of stock recovery
set -euo pipefail
cd "$(dirname "$0")/.."
SAM=scripts/samloader

cmd="${1:-help}"
case "$cmd" in
  fixusb)
    echo root | sudo -S $SAM fix-usb || true
    echo "If it asked for your Linux password instead, that's OK."; exit 0;;
  los)
    IMG=tmp/los-recovery.img && [ -f "$IMG" ];;
  creo)
    [ -f build/phone/boot.img ] || { echo "run 'make phone' first"; exit 1; }
    IMG=build/phone/boot.img;;
  restore)
    f=$(ls -1t backup/recovery-*.img 2>/dev/null | head -1 || true)
    [ -n "${f:-}" ] || { echo "no backup/recovery-*.img found"; exit 1; }
    IMG="$f";;
  *) sed -n '3,12p' "$0"; exit 0;;
esac

echo "== flashing $IMG -> RECOVERY partition"
echo "   Put the phone in DOWNLOAD MODE now (Vol- + Bixby + USB), press Continue."
sudo "$SAM" flash --no-reboot -p RECOVERY "$IMG"
echo
echo "DONE. To boot what we flashed: hold Vol+ + Bixby + Power (recovery)."
echo "To go back to Android: just power off and power on normally (boot untouched)."
