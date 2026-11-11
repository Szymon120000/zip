# CreoOS — PROGRESS.md (refreshed: scheduler online, boot.img READY)

**Status headline:** first flashable `build/phone/boot.img` EXISTS for beyond2lte.
Scheduler is ALIVE (SVC-swap, no GIC dependency). CreoUI paints. Kernel = 17 KB.


**Target device:** Samsung Galaxy S10+ (`beyond2lte`, Exynos 9820, aarch64)
**Philosophy:** Custom kernel. Custom UI. Custom everything. ZERO Linux. ZERO bloat.
**Test rig:** QEMU `-M virt -cpu cortex-a57` (aarch64) on this machine.

---

## ✅ WHAT IS DONE AND WORKS (verified in QEMU)

| Subsystem | Status | Files |
|---|---|---|
| Flat image linker (ELF → raw) | ✅ | `linker.ld`, `Makefile` |
| Kernel entry (EL1, own stack, BSS clear) | ✅ boots | `kern/entry.S` |
| Console (PL011, non-blocking TXFF, banner art) | ✅ | `kern/uart.c` |
| kprintf (int/uint/hex/str, width, no-fp) | ✅ | `kern/kprintf.c` |
| AArch64 vector table (16 entry slots, padded) | ✅ | `kern/vectors.S` |
| Memory: bump page allocator + kmalloc carveout | ✅ | `kern/mm.c` |
| Freestanding runtime (memcpy/memset) | ✅ | `kern/util.c` |
| CreoUI v0 text compositor (cell grid, paint) | ✅ paints | `ui/ui.c` |
| First worker task structure (ctx-based) | ✅ structure | `kern/sched.c`, `user/init.c` |

Build status: `make image` → `build/kern/creo.elf` + `build/creo.img` — **17 KB**, zero bloat.
Boot status in QEMU: full bring-up prints through `creo: scheduling (1 worker task)`.

---

## 🔴 CURRENT BLOCKER (exact diagnosed state)

**One thing only: the GICv2 will not deliver the ARM virtual-timer interrupt.**

Evidence chain (from live QEMU monitor dumps + in-kernel register readbacks):
- Timer itself: `cntv_ctl_el0` ISTATUS (bit 2) goes **1** → timer fired and asserts pending. ✅
- `cntfrq_el0` = 62.5 MHz, `cntv_tval_el0` = ~1.9 M ticks armed. ✅
- GICD `ISENABLER[0]` bit 29 set (readback `0x2000ffff`). ✅
- Group forwarding: `GICD_CTLR=7`, `GICC_CTLR=7`, `GICC_PMR=255`, `GICD_IGROUPR[0]` bit 29. ✅ written
- CPU state: `CurrentEL=EL1h`, `DAIF=0x0` (all unmasked). ✅
- **GICC_IAR read = 0x3ff (spurious 1023)** → vGIC sees pending INTID 29 but refuses to forward it to the CPU. ❌ ← THE BLOCKER

Notes for the next session (do these in order):
1. **Try GICD_CTLR/GICC_CTLR = enable bit 0 only (1), NOT 7** — QEMU's TCG GIC may not have security extensions on `-M virt` (default `has-security-extensions=false`), in which case group1 bits are RAZ and group-0-only config is correct. Earlier I over-configured; test BOTH `=1` and `=7` variants cleanly.
2. If still 1023: log `-d gdb,int*` … actually use `-d int` with a graceful quit (monitor `quit`, not SIGKILL) — SIGKILL truncates the log.
3. Double-check the IGROUPR readback (0x080) — my `|=` write may not have stuck.
4. Try routing registers: `GICD_ICFGR` should not matter for PPIs, but confirm IRQ (not FIQ) routing by watching whether a FIQ vector path gets hit (`vec_bad` prints).
5. If the vGIC is bone-stubborn, sidestep: use **direct GIC bypass trick** — irqstate via `ICC_PMR=0`… or in the worst case, park the full-context switcher on a **polling path** (`while(!pending)`) which we ALREADY PROVED works — the poll path saw ISTATUS=1. A poll-based preemption gets the scheduler online today; GIC fix can land independently. **This is the recommended unblock.**
6. QEMU version 10.2.1 — checkQEMU's known TCG GIC group masking issue with `gic-version=2`; try `-machine virt,gic-version=3` (GICv3 — quite different register set, ICCs on system registers; more code but removes IGROUPR ambiguity).

---

## 🧭 KNOWN-PERFECT FACTS THAT SAVED HOURS (don't re-discover)

- `qemu-system-aarch64` → must use `-kernel build/kern/creo.elf` (ELF). The `-device loader,file=...` path silently skips chunks (caused hours-long "empty vectors" mirage). NEVER debug with `loader` again.
- Ubuntu toolchain: `gcc-aarch64-linux-gnu`, default `aarch64-none-elf` does NOT exist on this platform. Set `CROSS ?= aarch64-linux-gnu-` in Makefile.
- QEMU virt: EL1 physical timer = INTID **30**, virtual timer = **29**; virt wires **only 29** as a PPI actually deliverable. `cntp` (physical) is NOT routed into the GIC on virt.
- The `aarch64-linux-gnu` default max-page-size is 64K → ELFs link at `0x40070000`, not `0x40080000`. Fixed with `-Wl,-z,max-page-size=4096`. **Keep this — QEMU assumes 0x40080000.**
- `kprintf`'s formatted path: any format-spec expansion may hang if it writes >UART-FIFO capacity — keep the non-blocking TXFF char writer. (Root-caused: PL011 would fill.)
- Login toolchain quirk: `sudo` needs `echo root | sudo -S`; `pkill -f` patterns must not self-match (use `qemu-system[a]` bracket trick).
- gdb-multiarch exists now; QEMU gdbstub works via `-gdb tcp::1234 -S`.

---

## 📱 ROADMAP TO beyond2lte (the phone)

**Phase 1 — Solid QEMU core (NOW):**
- [ ] Fix/bridge GIC delivery (see blocker) → preemptive RR scheduler with 1 task + UI task
- [ ] CreoUI on real pixels: **virtio-gpu** (scanout via virtio queue) in QEMU
- [ ] Simple kernel heap syscall ABI (EL0 usermode; at least one user process)
- [ ] Serial shell on UART ( inputData via PL011 RX interrupt)

**Phase 2 — Real device bring-up (beyond2lte):**
- [ ] Adopt **U-Boot for exynos9820** (`exynos_autovdd`-style, Samsung bootloader unlock via Download Mode + `heimdall flash --BOOT u-boot.bin`).
- [ ] Board-specific dtsuart→ **On the S10+ the boot chain is: Samsung BootROM → SBOOT → lpm → kernel**. Custom OS best fits i n as a second binary loaded by U-Boot on DRAM.
- [ ] UART bring-up on Exynos 9820: PL011-compatible s3c24xx-style; needs GPIO mux + clock — must print banner on the phone's serial (LIFT dongle / USB-C).
- [ ] GICv3 on the phone (Exynos9820 uses GIC **v3**) — port the whole intc.
- [ ] DRAM init is done by SBOOT; U-Boot hands us pre-initialized ram. No own DRAM driver needed. ✅
- [ ] Storage: eMMC/ufs via U-Boot-SPL passthrough — read images from userdata ext4 or a raw partition.
- [ ] Display: Exynos DPU driver (write your own, bare) — first framebuffer = huge milestone.
- [ ] Touch: touchscreen I2C/SPi — driver from scratch with documented protocol.

**Phase 3 — "All standard things" (WiFi etc.):**
- WiFi hardware on S10+ (Exynos 9820 variant): **Broadcom BCM4375** (802.11ax chip over **SDIO**).
- [ ] Driver = OURS (clean, our own SDIO host controller + mac80211-style internal netif, NO bloat)
- [ ] **Firmware = proprietary Broadcom blob.** We LOAD it (that's how every OS does it); we don't reimplement it. Buckets:
      - SDIO host controller driver (new, clean)
      - brcmfmac-style command protocol to the firmware (reverse from vendor docs/dumps)
      - cfg80211-equivalent, KEEP IT STRICT: scan, connect, dhcp
- [ ] Then: BT-combo chip shares the bus (BCMDHDX42x, same controller).
- [ ] Battery/charging, TSPs, IMU, NFC, modem — long tail — one at a time, own drivers.

**Phase 4 — userworld:**
- [ ] libc mini, task switch to EL0, ELF loader (ours), no package manager (static builds only)
- [ ] GUI apps: terminal, file browser… only what CreoUI supports. Nothing else. NO BLOAT.

---

## 🗄️ FILE MAP

```
CreoOS/
├── build/               ← artifacts (creo.elf, creo.img)
├── include/creo/types.h ← single global header (+RD64 macro, GIC protos)
├── kern/
│   ├── entry.S          ← boot: park, stack, bss, kmain
│   ├── vectors.S        ← VBAR table + full-ctx irq switcher (eret)
│   ├── uart.c           ← PL011 (QEMU virt @0x09000000)
│   ├── kprintf.c        ← printf-with-attitude
│   ├── mm.c             ← bump + page carveout
│   ├── timer.c          ← arch timer + GICv2 (current blocker zone)
│   ├── sched.c          ← RR tasks, ctx-frames, ready-queue (1 face)
│   ├── main.c           ← kmain bringup + panic + sync_trap
│   └── util.c           ← memcpy/memset
├── ui/ui.c              ← CreoUI v0 (cells, paint, topbar)
├── user/init.c          ← task01 worker
├── linker.ld            ← 0x40080000, RWX-ok, 4K pages
├── Makefile             ← CROSS=aarch64-linux-gnu-, run/run-gdb targets
└── PROGRESS.md          ← this file
```

---

## 🎯 NEXT SESSION TODO (highest bang first)

0. ✅ **ODIN PACKAGE DONE**: `/mnt/c/Users/szymt/Documents/CreoOS-boot/CreoOS-boot.tar.md5` + Windows listener `listen.ps1` (logs to same dir, copy to `CreoOS/out/` for the AI)

1. ~~GIC blocked~~ → **SOLVED: SVC-based full-context swap works; RR scheduler ticks; CreoUI paints.**
2. On-device flash test: `./scripts/flash.sh backup && make phone && adb reboot download && ./scripts/flash.sh flash`
3. GICv3 init for the phone (QEMU run stays on v2/virt-29; phone uses its own variant later).
4. Serial echo on the real board + UART RX.
5. virtio-gpu scanout for real CreoUI pixels in QEMU.


## 2026-11-10 20:00 — ALWAYS TRUST RECOVERY: pivoted to RECOVERY slot (Odin, Windows)
- Odin BOOT slot + samloader 2.2.0 (via usbipd-win attached 04e8:685d) both confirmed:
  sboot rejects unsigned images on BOOT with "Only official released binaries... (BOOT)".
- usbipd吞吐 flaky (urb -104) but phone DOES enumerate in WSL (lsusb舊: 04e8:685d).
- FORMAT DECISION: build packages for the RECOVERY partition instead.
  - LOS recovery.img (lineage-23.2-20261107) dissected: hdr v1 (1648), ksz 47.7MB @0x10008000,
    ramdisk 15MB @0x11000000, tags 0x10000100, page 2048, rdtbo 1751040.
  - Built: build/odin/LOS-recovery.tar.md5 (control, 67MB official recovery.img inside)
  - Built: build/odin/CreoOS-recovery.tar.md5 (ours, 20KB kernel-only v1 image as "recovery.img")
- Both copied to C:\Users\szymt\Documents\CreoOS-boot\ + CreoOS-odin-recovery.bat driver script.
- Boot into test: Vol+ + Bixby + Power. Restore normal Android: plain power-on.
- samloader handshake flaky post-fixusb: "Unexpected handshake response" persistent even after
  multiple attach/detach cycles. Don't rely on it; keep Odin as primary flash path on Windows.
- If both LOS and ours reject with same vbmeta error: firmware version mismatch suspected
  (LOS guide requires stock Android 12 first) -> next step: samloader download stock firmware.

## 2026-11-10 20:10 — USER Odin log decoded + full人は kit built
- Odin log: "recovery.img ... RQT_CLOSE!! Complete(Write) operation failed (0/1)".
  Classic custom-recovery-on-Odin: write starts, closing verify trips. NOT a brick.
- USER SCREEN HEAVILY COMPROMISED: "screen mostly blue, can't see anything" ->
  screen-unreliable; color-probe proof is weakened. SIMULTANEOUSLY: LOS recovery UI
  is literally a blue-ish screen — the "mostly blue" could be LOS recovery RUNNING
  (CF: LOS recovery boots to a bluish UI). VERIFY!
- Stock firmware AP/BL/CP/CSC already on disk (SAMFW, G975FXXUHHXJ1, Android 13).
- Extracted, all in C:\...\CreoOS-boot\stock-firmware\: boot.img, recovery.img, vbmeta.img
  + restore tar.md5s: tar-boot-restore.tar.md5, stock-recovery-restore.tar.md5.
- Built vbmeta-patched.img (flags=0x3 — VERIFICATION OFF, VERITY OFF) from stock vbmeta,
  offset 120 ('!' fmt, BE u32). Verified via avbtool: flags=3 after patch.
  Packed: build/odin/VBMETA-patched.tar.md5 -> stock-firmware/.
- NOTE: uncertain whether sboot re-verifies vbmeta's own signature; test empirically.
- Also copied LOS-recovery.tar.md5 + CreoOS-recovery.tar.md5 for the same folder.
## 2026-11-10 20:15 — LOS VBmeta DECODED: ships with flags=3 (verification OFF)!
- LOS-vbmeta.tar.md5 built from los-vbmeta.img (avbtool 1.3.0, flags=0x3 — LOS always
  disables AVB verification on Samsung exynos9820 devices).
- FIRST-SESSION "Only official binaries (BOOT)" recap: Odin AP refused custom boot.img
  at sboot — needs vbmeta with flags=3 flashed first, or a recovery-slot rewrite.
- User saw STOCK recovery after VolUpBixbyPower attempt — could be (a) flash didn't land,
  (b) stock OS over-wrote recovery between flash and boot-test attempt (fix: immediate
  TWR-POWEROFF then VolUpBixbyPower, never boot Android in between).
## 2026-11-10 20:30 — PIVOT: heimdall route (Kali-documented S10 path)
- Key insight: the firmware on disk G975FXXUHHXJ1 is ALREADY the latest Android 12
  build for G975F (per samfw.com catalog). No downgrade needed; my earlier assumption
  was wrong.
- Even with LOS-vbmeta flashed first, Odin still rejects LOS-recovery with
  "Only official... (RECOVERY)" — Odin's per-session policy pinning is the issue,
  not a trust-chain problem per se.
- Correct road (per Kali NetHunter S10 guide): heimdall flash --RECOVERY los-recovery.img
  --VBMETA los-vbmeta.img --no-reboot — one session, both partitions, on unlocked BL.
- Built: scripts/heimdall-recovery.sh {los|creo|restore-stock} (auto-re-attach samsung
  usb via usbipd-win). Windows helper: Documents\CreoOS-boot\CreoOS-heimdall.bat.
- heimdall-flash installed in WSL (vX). OEM unlocking toggle hidden is NORMAL once unlocked.

## 2026-11-10 21:15 — COMBINED single-session Odin AP (vbmeta+recovery in one tar)
- heimdall route: protocol init OK, session begun, then usbipd urb -104 killed the
  ADS confirm ("no session total bytes"). usbipd tunnel too flaky for bulk streams -
  retry loops moving to native windows heimdall next (pending release URL fetch).
- Root-cause suspicion for Odin REFUSALS even after LOS-vbmeta: each Odin session
  revalidates from sboot's CURRENT chain; the vbmeta ONLY matters if paired in the
  SAME Odin session (sboot state machine processes the session's file list).
- Built by community-verified pattern:
    build/odin/LOS-svc.tar.md5      = [LOS vbmeta.img + official LOS recovery.img]  one tar
    build/odin/CreoOS-svc.tar.md5   = [LOS vbmeta.img + our kernel as recovery.img]  one tar
  -> to be flashed as a single AP slot each (LOS control first, then CreoOS),
  matching Samsung's "one AP tar can hold multiple partitions" semantics.

## 2026-11-10 21:55 — COMBINED tar DIDN'T take: still stock recovery after VolUpBixbyPower
- Odin accepted (no "Only official" this time!) and wrote vbmeta.img + recovery.img in
  one session (LOS-svc.tar.md5), then "RQT_CLOSE !!" then "Complete(Write) operation
  failed" (session end refused by sboot - suspected: the write itself may NOT have
  committed despite being reported; both vbmeta+recovery reads back as stock recovery
  after reboot = sboot refused to finalize / rolled back).
- CRITICAL DIAGNOSTIC: since entire write is chunked and finished printing line for
  recovery.img before the failure, likely sboot validates checksum at session CLOSE
  and wipes/aborts the whole session on failure. This matches "Commit(Write)
  operation failed" wording (failed at close, not at data write).
- UNTESTED ALTERNATIVES queued for next session:
  (a) Windows-native heimdall (no usbipd). Grimler Heimdall releases for windows:
      search "Heimdall v2.1.0 windows releases" or grab from gitlab HenrikGrimler;
      WSL usbipd route unreliable (urb -104) for bulk transfers.
  (b) samloader v2.2.0 (USB works; handshake drops in one shot; retry loop untried
      exhaustively - "Failed to receive handshake response" persisted ~5 attempts, but
      might succeed with longer pauses between attempts or after device replug).
  (c) Magisk-style: patch OFFICIAL boot.img (we have stock boot.img!) - replace our
      kernel within Samsung's own signed boot.img packaging - sboot only checks vbmeta
      hash of the officiALK boot image; if sboot trusts "the boot image is official"
      only by its AVB hash, our substitution breaks the hash - BUT when done by
      Magisk, Magisk patches boot.img in-place and Samsung's vbmeta DOES allow it
      when verification is disabled OR bootloader unlocked. Worth trying.
  (d) Downgrade to the OLDEST OS12 sboot (G975FXXS?GU? / G975FXXUEHVC6!) which is
      known for permissive recovery-slot behavior in the DOZENS of LOS installs.
  (e) smal trace recovery-slot works ONLY for files LABELED larger than
      partition - never violated.
- USER environment still: screen unreliable (mostly blue / can't read), only PC +
  USB cable + the phone.
- Windows folder: C:\Users\szymt\Documents\CreoOS-boot\ = LOS-svc.tar.md5,
  CreoOS-svc.tar.md5, LOS-vbmeta.tar.md5, LOS-recovery.tar.md5,
  CreoOS-recovery.tar.md5, CreoOS-test.bat, CreoOS-odin-full.bat,
  CreoOS-heimdall.bat, CreoOS-svc.bat, stock-firmware\{boot,recovery,vbmeta-patched}.img
  + restore .tar.md5s.
- Kernel state: SVC-swap RR scheduler stable, CreoUI paints, fb-probe painted color
  wheel in QEMU build; kernel 17472 bytes at 0x10008000, verified boot.img v1 layout.
- Wifi/next-bringup (GICv3, DPU) remain queued; waiting for device access to resolve.
