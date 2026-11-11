#!/usr/bin/env bash
# CreoOS — heimdall path for beyond2lte (Kali-documented S10 route).
# Flash LineageOS recovery + LOS vbmeta in ONE protocol session.
# usage: ./scripts/heimdall-recovery.sh { los | creo | restore-stock }
set -uo pipefail
cd "$(dirname "$0")/.."

# auto re-attach samsung usb from the Windows host
BUSID=$(/mnt/c/Program\ Files/usbipd-win/usbipd.exe list 2>/dev/null | grep '04e8:685d' | awk '{print $1}')
if [ -n "$BUSID" ]; then
  echo "== re-attaching samsung usb ($BUSID) into WSL"
  /mnt/c/Program\ Files/usbipd-win/usbipd.exe attach --wsl --busid "${BUSID%%[[:space:]]*}" 2>&1 | tail -1
  sleep 3
fi
command -v heimdall >/dev/null || { echo "heimdall missing"; exit 1; }

case "${1:-}" in
  los)
    echo "== heimdall: LOS recovery + LOS vbmeta -> RECOVERY + VBMETA"
    sudo heimdall flash --RECOVERY tmp/los-recovery.img --VBMETA los-vbmeta.img --no-reboot
    ;;
  creo)
    echo "== heimdall: CreoOS custom kernel (recovery.img) -> RECOVERY+VBMETA"
    sudo heimdall flash --RECOVERY build/phone/boot.img --VBMETA los-vbmeta.img --no-reboot
    ;;
  restore-stock)
    echo "== heimdall: restore STOCK recovery + VBMETA"
    sudo heimdall flash --RECOVERY tmp/stock/recovery.img --VBMETA los-vbmeta.img --no-reboot
    ;;
  *) echo "usage: $0 {los|creo|restore-stock}"; exit 0;;
esac
echo
echo "DONE. Do NOT boot Android. Power off (VolDown+Power ~8s), then immediately"
echo "boot into recovery: Vol UP + Bixby + Power."
