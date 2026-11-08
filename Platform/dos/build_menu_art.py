#!/usr/bin/env python3
"""Build compact native DOS title graphics from the supplied Space Cadet image.

The owner's 600x600 Space Cadet splash was resized and RGBI-quantized offline,
then stored as a small compressed, base64 text asset so building needs only
Python's standard library. MENU.DAT works in both standard VGA 13h and
Tandy/PCjr mode 09h; no palette conversion or file I/O happens during play.
"""
import base64
import hashlib
from pathlib import Path
import struct
import sys
import zlib

HERE = Path(__file__).resolve().parent
WIDTH = 240
HEIGHT = 200
ROW_BYTES = WIDTH // 2
LOGO_WIDTH = 76
LOGO_HEIGHT = 53
BACKGROUND_BYTES = ROW_BYTES * HEIGHT
LOGO_BYTES = (LOGO_WIDTH // 2) * LOGO_HEIGHT
RAW_BYTES = BACKGROUND_BYTES + LOGO_BYTES
EXPECTED_SHA256 = "69a15a865173c04a69f357598dce31208248eb45934c0c397a153832191930b8"
PALETTE = (
    (0,0,0), (0,0,170), (0,170,0), (0,170,170),
    (170,0,0), (170,0,170), (170,85,0), (170,170,170),
    (85,85,85), (85,85,255), (85,255,85), (85,255,255),
    (255,85,85), (255,85,255), (255,255,85), (255,255,255),
)


def decode_source():
    encoded = (HERE / "data" / "menu_art.b64").read_text(encoding="ascii")
    raw = zlib.decompress(base64.b64decode(encoded, validate=False))
    if len(raw) != RAW_BYTES:
        raise ValueError(f"Invalid packed title size: {len(raw)}, expected {RAW_BYTES}")
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED_SHA256:
        raise ValueError("Title asset checksum mismatch")
    return raw


def unpack_rgbi(packed, width):
    """Expand one row of high-nibble-first RGBI pixels for previews/tests."""
    if len(packed) * 2 != width:
        raise ValueError("Packed row size mismatch")
    return [value for byte in packed for value in (byte >> 4, byte & 15)]


def png_bytes(pixels):
    """Generate an RGB PNG using only Python stdlib; 320x200 menu preview."""
    width, height = 320, 200
    if len(pixels) != width * height:
        raise ValueError("Preview size mismatch")

    def chunk(name, data):
        return (struct.pack(">I", len(data)) + name + data +
                struct.pack(">I", zlib.crc32(name + data) & 0xffffffff))

    scanlines = bytearray()
    for y in range(height):
        scanlines.append(0)
        for index in pixels[y*width:(y+1)*width]:
            scanlines.extend(PALETTE[index])

    return (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(scanlines, 9)) +
            chunk(b"IEND", b""))


def build(out_dir):
    raw = decode_source()
    dest = Path(out_dir)
    dest.mkdir(parents=True, exist_ok=True)
    (dest / "MENU.DAT").write_bytes(b"SCM1" + raw)

    # The right 80 pixels are a live menu panel drawn by hud.inc at runtime.
    preview = [0] * (320 * 200)
    for y in range(HEIGHT):
        row = unpack_rgbi(raw[y*ROW_BYTES:(y+1)*ROW_BYTES], WIDTH)
        preview[y*320:y*320+WIDTH] = row
    (dest / "MENU_PREVIEW.png").write_bytes(png_bytes(preview))
    return dest / "MENU.DAT"


if __name__ == "__main__":
    output = sys.argv[1] if len(sys.argv) > 1 else str(HERE / "build")
    print("Space Cadet title:", build(output))
