#!/usr/bin/env python3
"""Pack the client's fonts and language packs into one blob for the executable.

    pack_assets.py <out.bin> <out.S> <port dir>

Layout written to out.bin:
    "LAINAS01" | u32 count | count * (u32 path_len, path, u32 size, data)

Paths are relative to the client's data directory: fonts/ui.ttf,
lang/zh/subtitles/..., so the client can write them next to settings.ini on
first run. out.S is a .incbin of the blob; the client reads the two boundary
symbols.
"""
import os
import struct
import sys

MAGIC = b"LAINAS01"


def asset_roots(port_dir):
    """(source dir, prefix under the data dir)."""
    roots = [(os.path.join(port_dir, "fonts"), "fonts")]
    # Launcher artwork (client/port assets): written next to settings.ini.
    assets = os.path.join(port_dir, "assets")
    if os.path.isdir(assets):
        roots.append((assets, ""))
    packs = os.path.join(port_dir, "packs")
    if os.path.isdir(packs):
        for name in sorted(os.listdir(packs)):
            path = os.path.join(packs, name)
            if os.path.isdir(path) and name != "_originals":
                roots.append((path, "lang/" + name))
    return roots


def main():
    if len(sys.argv) != 4:
        sys.exit("usage: pack_assets.py <out.bin> <out.S> <port dir>")
    out_bin, out_asm, port_dir = sys.argv[1:4]

    entries = []
    for root, prefix in asset_roots(port_dir):
        if not os.path.isdir(root):
            continue
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames.sort()
            for name in sorted(filenames):
                if name.startswith("."):
                    continue
                full = os.path.join(dirpath, name)
                rel = os.path.relpath(full, root).replace(os.sep, "/")
                entries.append(((prefix + "/" + rel) if prefix else rel, full))

    with open(out_bin, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<I", len(entries)))
        for rel, full in entries:
            with open(full, "rb") as src:
                data = src.read()
            path = rel.encode("utf-8")
            f.write(struct.pack("<I", len(path)))
            f.write(path)
            f.write(struct.pack("<I", len(data)))
            f.write(data)

    # Both labels: ELF/MinGW use the bare name, Mach-O adds an underscore.
    asm = (
        "#ifdef __APPLE__\n"
        ".section __TEXT,__const\n"
        "#else\n"
        ".section .rodata\n"
        "#endif\n"
        ".balign 16\n"
        ".globl lain_assets_blob\n"
        ".globl _lain_assets_blob\n"
        "lain_assets_blob:\n"
        "_lain_assets_blob:\n"
        '.incbin "%s"\n'
        ".globl lain_assets_blob_end\n"
        ".globl _lain_assets_blob_end\n"
        "lain_assets_blob_end:\n"
        "_lain_assets_blob_end:\n"
        ".balign 16\n"
    ) % out_bin.replace("\\", "/")
    with open(out_asm, "w", encoding="utf-8", newline="\n") as f:
        f.write(asm)

    total = os.path.getsize(out_bin)
    print("embedded %d files, %d bytes" % (len(entries), total))


if __name__ == "__main__":
    main()
