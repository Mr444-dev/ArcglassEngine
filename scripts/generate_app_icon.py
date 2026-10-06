"""Create the Arcglass Engine multi-resolution Windows application icon."""
from __future__ import annotations

import math
import struct
import zlib
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
OUT = REPOSITORY_ROOT / "engine" / "platform" / "resources" / "Arcglass.ico"


def segment_distance(px: float, py: float, ax: float, ay: float, bx: float, by: float) -> float:
    dx, dy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / max(dx * dx + dy * dy, 1e-8)))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def rounded_box_distance(x: float, y: float, half: float, radius: float) -> float:
    qx, qy = abs(x) - (half - radius), abs(y) - (half - radius)
    return math.hypot(max(qx, 0.0), max(qy, 0.0)) + min(max(qx, qy), 0.0) - radius


def png_chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)


def make_png(size: int) -> bytes:
    rows = bytearray()
    scale = float(size)
    segments = [
        ((0.23, 0.77), (0.50, 0.19), (0.18, 0.82, 1.00)),
        ((0.50, 0.19), (0.79, 0.77), (0.69, 0.79, 1.00)),
        ((0.36, 0.56), (0.65, 0.56), (0.39, 0.72, 1.00)),
        ((0.23, 0.77), (0.79, 0.77), (0.31, 0.59, 0.95)),
    ]
    line_width = 0.054
    for y in range(size):
        rows.append(0)
        for x in range(size):
            u, v = (x + 0.5) / scale, (y + 0.5) / scale
            dx, dy = u - 0.5, v - 0.5
            edge = rounded_box_distance(dx, dy, 0.475, 0.17)
            if edge > 0:
                rows.extend((0, 0, 0, 0))
                continue
            shade = max(0.0, min(1.0, 0.5 + 0.35 * (0.5 - v) + 0.25 * (0.5 - u)))
            color = [int(10 + 12 * shade), int(17 + 22 * shade), int(36 + 34 * shade)]
            glow = 0.0
            for (a, b, rgb) in segments:
                d = segment_distance(u, v, a[0], a[1], b[0], b[1])
                halo = max(0.0, 1.0 - d / (line_width * 2.7)) * 0.16
                core = max(0.0, min(1.0, (line_width + 0.75 / scale - d) * scale))
                glow = max(glow, halo)
                if core > 0:
                    color = [int(color[i] * (1.0 - core) + rgb[i] * 255.0 * core) for i in range(3)]
            if glow > 0:
                color = [min(255, int(c + glow * tint)) for c, tint in zip(color, (32, 94, 150))]
            rows.extend((*color, 255))
    raw = b"\x89PNG\r\n\x1a\n"
    raw += png_chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0))
    raw += png_chunk(b"IDAT", zlib.compress(bytes(rows), 9))
    raw += png_chunk(b"IEND", b"")
    return raw


def main() -> None:
    images = [make_png(size) for size in (16, 24, 32, 48, 64, 128, 256)]
    header_size = 6 + 16 * len(images)
    directory = bytearray(struct.pack("<HHH", 0, 1, len(images)))
    offset = header_size
    for size, image in zip((16, 24, 32, 48, 64, 128, 256), images):
        directory.extend(struct.pack("<BBBBHHII", size & 255, size & 255, 0, 0, 1, 32, len(image), offset))
        offset += len(image)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_bytes(bytes(directory) + b"".join(images))
    print(f"Generated {OUT} ({OUT.stat().st_size} bytes, 7 PNG resolutions)")


if __name__ == "__main__":
    main()
