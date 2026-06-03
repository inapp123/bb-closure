#!/usr/bin/env python3
"""Convert images to BB demo LZO-compressed C source files.

Usage:
    uv run convert.py input.png fk1
    uv run convert.py input.png fk1 --prefix fk --height 200

The script generates a .c file matching the format used by BB demo.
Each person has 4 photos (e.g. fk1, fk2, fk3, fk4).
"""

import argparse
import subprocess
from pathlib import Path

from PIL import Image


HEADER = """\
/*
 * BB: The portable demo
 *
 * (C) 1997 by AA-group (e-mail: aa@horac.ta.jcu.cz)
 *
 * 3rd August 1997
 * version: 1.2 [final3]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public Licences as by published
 * by the Free Software Foundation; either version 2; or (at your option)
 * any later version
 *
 * This program is distributed in the hope that it will entertaining,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILTY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Publis License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*Automatically generated image {name} */
#include "image.h"
"""


MINILZO_DIR = Path(__file__).resolve().parent.parent / "src" / "lzo"
COMPRESSOR_SRC = Path(__file__).resolve().parent / "minilzo_compress.c"
COMPRESSOR_BIN = Path(__file__).resolve().parent / "minilzo_compress"


def ensure_compressor() -> Path:
    """Build the minilzo compressor used by the BB demo decompressor."""
    needs_build = not COMPRESSOR_BIN.exists()
    if not needs_build:
        src_mtime = max(COMPRESSOR_SRC.stat().st_mtime, (MINILZO_DIR / "minilzo.c").stat().st_mtime)
        needs_build = COMPRESSOR_BIN.stat().st_mtime < src_mtime
    if needs_build:
        subprocess.run(
            [
                "gcc",
                "-O2",
                str(COMPRESSOR_SRC),
                str(MINILZO_DIR / "minilzo.c"),
                f"-I{MINILZO_DIR}",
                "-o",
                str(COMPRESSOR_BIN),
            ],
            check=True,
        )
    return COMPRESSOR_BIN


def lzo_compress(raw: bytes) -> bytes:
    """Compress with lzo1x_1, matching minilzo's lzo1x_decompress in BB."""
    compressor = ensure_compressor()
    result = subprocess.run([str(compressor)], input=raw, capture_output=True, check=True)
    return result.stdout


def prepare_image(image_path: str, height: int) -> Image.Image:
    img = Image.open(image_path)
    if img.mode == "RGBA":
        background = Image.new("RGBA", img.size, (0, 0, 0, 255))
        img = Image.alpha_composite(background, img)
    img = img.convert("L")

    w, h = img.size
    new_w = round(w * height / h)
    img = img.resize((new_w, height), Image.LANCZOS)
    return img


def image_to_c(image_path: str, name: str, height: int = 200) -> str:
    img = prepare_image(image_path, height)
    new_w, height = img.size

    raw = img.tobytes()
    compressed = lzo_compress(raw)

    # Build C array
    lines = [HEADER.format(name=name)]
    lines.append(f"static unsigned char {name}data[] =")
    lines.append("{")

    # Format bytes as comma-separated values, 16 per line
    byte_strs = [str(b) for b in compressed]
    for i in range(0, len(byte_strs), 16):
        chunk = ", ".join(byte_strs[i : i + 16])
        if i + 16 < len(byte_strs):
            lines.append(f"    {chunk},")
        else:
            lines.append(f"    {chunk},")

    lines.append("};")
    lines.append(f"struct image {name} =")
    lines.append("{" + f"{name}data, {len(compressed)}, {new_w}, {height}" + "};")
    lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Convert image to BB demo C source")
    parser.add_argument("image", help="Input image path")
    parser.add_argument("name", help="Variable name (e.g. fk1, ms2, kt3, hh4)")
    parser.add_argument("--height", type=int, default=200, help="Target height (default: 200)")
    parser.add_argument("-o", "--output", help="Output .c file (default: <name>.c in script dir)")
    args = parser.parse_args()

    output = args.output or str(Path(__file__).parent / f"{args.name}.c")
    c_code = image_to_c(args.image, args.name, args.height)
    Path(output).write_text(c_code)
    print(f"Written {output}")


if __name__ == "__main__":
    main()
