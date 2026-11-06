#!/usr/bin/env python3
"""Check the final target image and prove the TEST83 .voice_hot omission is caught."""
import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "fm1/base/tools"))
from check_image_layout import APP_XIP, audit_image_layout  # noqa: E402

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("elf", type=Path)
parser.add_argument("image", type=Path)
args = parser.parse_args()
elf, image = args.elf.read_bytes(), args.image.read_bytes()
regions = audit_image_layout(elf, image)
hot = next((r for r in regions if r["name"] == ".voice_hot"), None)
if hot is None or hot["size"] == 0:
    raise SystemExit("FAIL: RC1 must include a nonempty .voice_hot section")
broken = bytearray(image)
offset = hot["load"] - APP_XIP
broken[offset:offset + hot["size"]] = b"\xff" * hot["size"]
try:
    audit_image_layout(elf, broken)
except ValueError as exc:
    if ".voice_hot:" not in str(exc):
        raise SystemExit(f"FAIL: negative control failed for an unrelated reason: {exc}")
    print(f"PASS negative control: omitted .voice_hot rejected ({exc})")
else:
    raise SystemExit("FAIL: the image checker accepted an omitted .voice_hot section")
print(f"PASS final target image: {len(regions)} initialized ELF sections match; .voice_hot {hot['size']} B")
