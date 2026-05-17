#!/usr/bin/env python3
"""Convert images to BB demo LZO-compressed C source files.

Usage:
    uv run convert.py input.png fk1
    uv run convert.py input.png fk1 --prefix fk --height 200

The script generates a .c file matching the format used by BB demo.
Each person has 4 photos (e.g. fk1, fk2, fk3, fk4).
"""

import argparse
import sys
from pathlib import Path

import lzo
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


def image_to_c(image_path: str, name: str, height: int = 200) -> str:
    img = Image.open(image_path).convert("L")

    # Scale to target height, keep aspect ratio
    w, h = img.size
    new_w = round(w * height / h)
    img = img.resize((new_w, height), Image.LANCZOS)

    # Raw grayscale pixels
    raw = img.tobytes()

    # LZO compress (use lzo1x_999 for best compression, matching original)
    compressed = lzo.compress(raw, 9)

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
