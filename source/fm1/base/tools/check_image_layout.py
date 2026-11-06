#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Verify application image bytes against ELF load segments and startup copy spans.

Read the linked ELF directly rather than trusting the objcopy fragments used by
the builder. This catches an initialized section silently omitted from the image,
including the separately linked .voice_hot executable-SRAM section.
"""
import argparse
import struct
from pathlib import Path

APP_XIP = 0x02000120
RAMTEXT_START, RAMTEXT_END = 0x01C00000, 0x01C06000


def _cstring(table, offset):
    if not 0 <= offset < len(table):
        raise ValueError("ELF string offset is outside its table")
    end = table.find(b"\0", offset)
    if end < 0:
        raise ValueError("unterminated ELF string")
    return table[offset:end].decode("ascii")


def _read_layout(elf):
    if elf[:7] != b"\x7fELF\x01\x01\x01":
        raise ValueError("expected a little-endian ELF32 image")
    header = struct.unpack_from("<16sHHIIIIIHHHHHH", elf)
    phoff, shoff = header[5:7]
    phentsize, phnum, shentsize, shnum, shstrndx = header[9:14]
    if phentsize < 32 or shentsize < 40 or not 0 < shstrndx < shnum:
        raise ValueError("invalid ELF table geometry")
    sections = [struct.unpack_from("<10I", elf, shoff + i * shentsize) for i in range(shnum)]
    string_section = sections[shstrndx]
    names = elf[string_section[4]:string_section[4] + string_section[5]]
    segments = [struct.unpack_from("<8I", elf, phoff + i * phentsize) for i in range(phnum)]
    regions, symbols = [], {}
    for section in sections:
        name = _cstring(names, section[0])
        typ, flags, address, offset, size, link, _, _, entsize = section[1:]
        if typ == 2:  # SHT_SYMTAB; the startup copy boundaries are linker symbols.
            if not 0 <= link < len(sections) or entsize < 16 or size % entsize:
                raise ValueError("invalid ELF symbol table")
            strings = sections[link]
            table = elf[strings[4]:strings[4] + strings[5]]
            for pos in range(offset, offset + size, entsize):
                st_name, value, _, _, _, index = struct.unpack_from("<IIIBBH", elf, pos)
                if index:  # omit undefined symbols
                    symbols[_cstring(table, st_name)] = value
        if not flags & 2 or typ == 8 or not size:  # SHF_ALLOC, excluding SHT_NOBITS
            continue
        contents = elf[offset:offset + size]
        if len(contents) != size:
            raise ValueError(f"{name}: section extends beyond ELF file")
        loads = {p[3] + address - p[2] for p in segments
                 if p[0] == 1 and p[2] <= address and address + size <= p[2] + p[4]
                 and offset == p[1] + address - p[2]}
        if len(loads) != 1:
            raise ValueError(f"{name}: cannot resolve a unique PT_LOAD address")
        regions.append({"name": name, "address": address, "load": loads.pop(),
                        "size": size, "contents": contents})
    return regions, symbols


def audit_image_layout(elf, image, app_xip=APP_XIP):
    """Return checked initialized regions; raise ValueError on any missing byte."""
    try:
        regions, symbols = _read_layout(elf)
    except (struct.error, IndexError, UnicodeError) as exc:
        raise ValueError(f"invalid ELF: {exc}") from exc
    if not regions or ".text" not in {r["name"] for r in regions}:
        raise ValueError("ELF has no initialized application text")
    for region in regions:
        offset, size = region["load"] - app_xip, region["size"]
        name = region["name"]
        if offset < 0 or offset + size > len(image):
            raise ValueError(f"{name}: load range {region['load']:#x}+{size:#x} is absent from image")
        actual, expected = image[offset:offset + size], region["contents"]
        if actual != expected:
            bad = next(i for i, (a, b) in enumerate(zip(actual, expected)) if a != b)
            raise ValueError(f"{name}: image differs from ELF at load address {region['load'] + bad:#x}")

    by_name = {r["name"]: r for r in regions}
    for name, prefix in ((".ram_text", "_rt"), (".voice_hot", "_voice_hot"), (".data", "_data")):
        keys = [prefix + suffix for suffix in ("_start", "_end", "_load")]
        if not any(k in symbols for k in keys):
            if name in by_name:
                raise ValueError(f"{name}: startup copy symbols are missing")
            continue
        if not all(k in symbols for k in keys):
            raise ValueError(f"{name}: incomplete startup copy symbols")
        start, end, load = (symbols[k] for k in keys)
        if any(v % 4 for v in (start, end, load)) or end < start:
            raise ValueError(f"{name}: invalid word-aligned startup copy range")
        if end == start and name not in by_name:
            continue
        region = by_name.get(name)
        if not region or (start, end - start, load) != (region["address"], region["size"], region["load"]):
            raise ValueError(f"{name}: startup copy symbols disagree with ELF load mapping")

    sram = [by_name[n] for n in (".ram_text", ".voice_hot") if n in by_name]
    for region in sram:
        if not RAMTEXT_START <= region["address"] < region["address"] + region["size"] <= RAMTEXT_END:
            raise ValueError(f"{region['name']}: executable SRAM range is outside RAMTEXT")
    if len(sram) == 2:
        first, second = sram
        if first["address"] + first["size"] != second["address"]:
            raise ValueError("executable SRAM sections are not contiguous")
        if first["load"] + first["size"] != second["load"]:
            raise ValueError("executable SRAM flash load ranges are not contiguous")
    return [{k: v for k, v in r.items() if k != "contents"} for r in regions]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("image", type=Path)
    args = parser.parse_args()
    try:
        regions = audit_image_layout(args.elf.read_bytes(), args.image.read_bytes())
    except (OSError, ValueError) as exc:
        raise SystemExit(f"FAIL ELF/image layout: {exc}")
    for r in regions:
        print(f"PASS {r['name']}: {r['size']} B, load {r['load']:#010x}, run {r['address']:#010x}, exact ELF bytes")
    print("PASS startup copy spans match the ELF section sizes, load addresses and word alignment")


if __name__ == "__main__":
    main()
