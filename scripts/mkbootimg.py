#!/usr/bin/env python3
"""mkbootimg.py — Android boot image v1 writer (AOSP bootimg.h-accurate).

Layout (page-aligned, page 2048 for exynos9820):
  [ header 1 page ][ kernel n pages ][ ramdisk ][ second ][ recovery_dtbo ]

beyond2lte defaults (from LineageOS exynos9820-common BoardConfig9820.mk):
  base 0x10000000, kernel_offset 0x8000 -> kernel_addr 0x10008000
  ramdisk_offset 0x1000000 -> 0x11000000, tags_offset 0x100.
"""
import struct, sys, os

PAGE = 2048
KERNEL_ADDR = 0x10008000
RAMDISK_ADDR = 0x11000000
SECOND_ADDR = 0x10000000
TAGS_ADDR = 0x10000100
HEADER_V1_SIZE = 1648  # magic+9u32+name16+cmdline512+id32+extra1024+dtbo(4+8)+hdrsz4
                       # = 8+36+16+512+32+1024+4+8+4
OS_VERSION = 0  # build-agnostic; no os-version contract


def main(argv):
    if "--kernel" not in argv:
        sys.exit("need --kernel file")
    kernel = open(argv[argv.index("--kernel") + 1], "rb").read()
    out_path = "build/phone/boot.img"
    if "--out" in argv:
        out_path = argv[argv.index("--out") + 1]
    cmdline = b""
    if "--cmdline" in argv:
        cmdline = argv[argv.index("--cmdline") + 1].encode()
    cmdline = cmdline[:512]
    cmdline += b"\0" * (512 - len(cmdline))

    ramdisk = b"", 0
    second = b"", 0

    def pages(n):
        return (n + PAGE - 1) // PAGE

    ksz = 1024 * (len(kernel) // 1024 + 1) if len(kernel) % 1024 else len(kernel)
    ksz = len(kernel)
    kn = pages(ksz)

    hdr = bytearray(HEADER_V1_SIZE)
    o = 0
    hdr[0:8] = b"ANDROID!"                     # magic
    o = 8
    struct.pack_into("<I", hdr, o, len(kernel)); o += 4     # kernel_size
    struct.pack_into("<I", hdr, o, KERNEL_ADDR); o += 4     # kernel_addr
    struct.pack_into("<I", hdr, o, 0);            o += 4    # ramdisk_size
    struct.pack_into("<I", hdr, o, RAMDISK_ADDR); o += 4    # ramdisk_addr
    struct.pack_into("<I", hdr, o, 0);            o += 4    # second_size
    struct.pack_into("<I", hdr, o, SECOND_ADDR);  o += 4    # second_addr
    struct.pack_into("<I", hdr, o, TAGS_ADDR);    o += 4    # tags_addr
    struct.pack_into("<I", hdr, o, PAGE);         o += 4    # page_size
    struct.pack_into("<I", hdr, o, 1);            o += 4    # header_version=1
    struct.pack_into("<I", hdr, o, OS_VERSION);   o += 4    # os_version
    hdr[o:o+16] = b"creos" + b"\0"*(16-5);        o += 16   # product name
    hdr[o:o+512] = cmdline;                       o += 512  # cmdline
    # id[8] left zero; extra_cmdline zero
    o += 32 + 1024
    struct.pack_into("<I", hdr, o, 0);            o += 4    # recovery_dtbo_size
    struct.pack_into("<Q", hdr, o, 0);            o += 8    # recovery_dtbo_offset
    struct.pack_into("<I", hdr, o, HEADER_V1_SIZE)          # header_size

    total = PAGE + kn * PAGE
    img = bytearray()
    img += hdr
    img += b"\0" * (PAGE - len(hdr))
    img += kernel
    img += b"\0" * (kn * PAGE - len(kernel))
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    open(out_path, "wb").write(img)
    print(f"== {out_path}: {len(img)} bytes (hdr page + {kn} kernel pages)")


if __name__ == "__main__":
    main(sys.argv)
