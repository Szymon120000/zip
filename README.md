# CreoOS — Custom OS. Custom Kernel. Custom UI. NO BLOAT.

**Target:** Samsung Galaxy S10+ international (codename `beyond2lte`, SoC Exynos 9820, aarch64).
**This repo:** a from-scratch aarch64 kernel — no Linux, no Preempt-nonsense, no buffering
machinery we don't call, no systemd, no 3000-line linker scripts.

## What exists right now

```
$ make run        # boots in QEMU (cortex-a57, virt board)
```

Working today:
- Custom exception vectors (VBAR_EL1, handwritten in asm)
- Full-context cooperative+preemptive scheduler via single timer IRQ path
- GICv2 init, EL1 physical timer, round-robin RR tick
- PL011 serial console + tiny `kprintf`
- Page/bump memory manager (no slab bloat, no deferred free)
- **CreoUI v0** — custom UI compositor skeleton (cell grid → will target fb via DPU)

## In progress
- DTB parser (needed to auto-locate UART/GIC/DPU on real devices)
- Real framebuffer (virtio-gpu on QEMU first, then beyond2lte DPU)
- Touch input (evdev-equivalent reader, our own — no Linux input layer)
- Usermode (EL0 tasks with a real syscall ABI)

## Boot path on the real phone
See `docs/boot-beyond2lte.md`.

## WiFi on beyond2lte
See `docs/wifi.md` — this is the single hardest part of the project and the plan
is written out. Firmware is proprietary Broadcom; we load it, we don't reimplement
it. Driver is OURS.
