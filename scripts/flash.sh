#!/usr/bin/env bash
# CreoOS — beyond2lte flash + restore helper (Samsung Download Mode / heimdall)
set -euo pipefail
cd "$(dirname "$0")/.."

IMG=build/phone/boot.img
[ -f "$IMG" ] || { echo "run 'make phone' first"; exit 1; }
command -v heimdall >/dev/null || { echo "heimdall not installed (sudo apt install heimdall-flash)"; exit 1; }

case "${1:-help}" in
  backup)
    echo "→ DOWNLOAD-MODE: VolumeDown on boot; accept with VolumeUp"
    heimdall download --output backup/boot-$(date +%s).img --BOOT
    ;;
  flash)
    echo "-> flASS: beyond2lte boot partition <- $(wc -c <$IMG) bytes"
    echo "→ DOWNLOAD-MODE: VolumeDown on boot; accept with VolumeUp"
    heimdall flash --BOOT "$IMG" --verbose
    ;;
  restore)
    f=$(ls -1t backup/boot-*.img 2>/dev/null | head -1) || true
    [ -n "${f:-}" ] || { echo "no backup found in backup/"; exit 1; }
    echo "restoring $f ..."
    heimdall flash --BOOT "$f" --verbose
    ;;
  *) cat <<EOF
usage: $0 {backup|flash|restore}
  backup   = safe-keep current boot partition (RUN FIRST)
  flash    = flash build/phone/boot.img
  restore  = re-flash your newest stored backup
EOF
    ;;
esac
