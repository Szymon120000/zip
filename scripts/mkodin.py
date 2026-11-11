#!/usr/bin/env python3
"""mkodin.py — pack CreoOS boot.img into an Odin-flashable .tar.md5 (AP slot).
Odin only repartitions with BL; a plain AP tar with boot.img inside is the
standard "flash kernel via AP" shape used by every Samsung custom-kernel pack.
Also drops a .tar.md5 whose md5 string is concatenated (Odin checks it).
"""
import tarfile, hashlib, sys, os, time

src = sys.argv[1] if len(sys.argv) > 1 else "build/phone/boot.img"
outdir = sys.argv[2] if len(sys.argv) > 2 else "build/odin"
arcname = sys.argv[3] if len(sys.argv) > 3 else os.path.basename(src)
name = arcname.replace(".img", "")
outbase = sys.argv[4] if len(sys.argv) > 4 else (f"CreoOS-{name}" if name != "boot" else "CreoOS-boot")
tar_path = os.path.join(outdir, outbase + ".tar")
os.makedirs(outdir, exist_ok=True)

with tarfile.open(tar_path, "w") as t:
    info = t.gettarinfo(src, arcname=arcname)
    info.mtime = 0
    info.uid = info.gid = 0
    info.uname = info.gname = ""
    with open(src, "rb") as f:
        t.addfile(info, f)

md5 = hashlib.md5(open(tar_path, "rb").read()).hexdigest()
md5_path = tar_path + ".md5"
# Odin .tar.md5: the full tar bytes, then the md5 appended as text at the end
with open(md5_path, "wb") as f:
    f.write(open(tar_path, "rb").read())
    f.write((md5 + f"  {outbase}.tar\n").encode())
print(f"== {tar_path} ({os.path.getsize(tar_path)} bytes)")
print(f"== {md5_path} (md5 {md5}) — Odin slot: {arcname}")
