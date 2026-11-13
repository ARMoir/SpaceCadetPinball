#!/usr/bin/env python3
"""Stage only the runtime files for the CADET artifact and release."""
from pathlib import Path
import shutil

RUNTIME_FILES = ("CADET.EXE", "TABLE.DAT", "TABLE.TGA", "TABLE3D.TGA", "TABLESRC.TGA", "VGA.DAT", "MENU.DAT")
ROOT = Path(__file__).resolve().parents[2]


def package(source, destination):
    # Validate every required file before staging anything. Never include a
    # user's SCORES.DAT, test executables, logs, previews or build tools.
    payload = {name: (source / name).read_bytes() for name in RUNTIME_FILES}
    if payload["CADET.EXE"][:2] != b"MZ":
        raise ValueError("CADET.EXE is not a DOS executable")
    if payload["TABLE.DAT"][:4] != b"SCP2":
        raise ValueError("Expected matching SCP2 table data")
    for name in ("TABLE.TGA", "TABLE3D.TGA", "TABLESRC.TGA"):
        if len(payload[name]) != 32768:
            raise ValueError("Wrong banked artwork size: " + name)
    if len(payload["MENU.DAT"]) != 26018 or payload["MENU.DAT"][:4] != b"SCM1":
        raise ValueError("Invalid Space Cadet menu graphics pack")
    from verify_vga_art import validate_pack
    validate_pack(payload["VGA.DAT"], payload["TABLE.DAT"])
    destination.mkdir(parents=True, exist_ok=True)
    if any(p.name not in RUNTIME_FILES for p in destination.iterdir()):
        raise ValueError("Runtime staging directory contains unexpected files")
    for name in RUNTIME_FILES:
        shutil.copyfile(source / name, destination / name)
    print("Runtime package: " + ", ".join(RUNTIME_FILES))


if __name__ == "__main__":
    package(ROOT / "build/dos", ROOT / "build/package/runtime")
