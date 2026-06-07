#!/usr/bin/env python3
"""Convert binary STL to BB demo patnik.h mesh format.

Usage:
    python3 stl2patnik.py input.stl ../src/tex/patnik.h
"""

import argparse
import math
import struct
from pathlib import Path

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

/* Automatically generated from {source} */

#define patniknFaces {n_faces}
POLYS patnikobj[]=
{{
"""

NORMAL_SCALE = 8192
TARGET_SIZE = 120.0


def rot_x(x, y, z, degrees):
    r = math.radians(degrees)
    c, s = math.cos(r), math.sin(r)
    return x, y * c - z * s, y * s + z * c


def rot_y(x, y, z, degrees):
    r = math.radians(degrees)
    c, s = math.cos(r), math.sin(r)
    return x * c + z * s, y, -x * s + z * c


def orient_vertex(x, y, z):
    """Stand the model up, then yaw 45 + 180° for nicer 4-way views."""
    x, y, z = rot_x(x, y, z, -90)
    x, y, z = rot_y(x, y, z, 45 + 180)
    return x, y, z


def read_stl(path: Path):
    with path.open("rb") as f:
        f.read(80)
        count = struct.unpack("<I", f.read(4))[0]
        triangles = []
        for _ in range(count):
            tri = f.read(50)
            v1 = struct.unpack("<3f", tri[12:24])
            v2 = struct.unpack("<3f", tri[24:36])
            v3 = struct.unpack("<3f", tri[36:48])
            triangles.append((v1, v2, v3))
    return triangles


def bounds(triangles):
    xs, ys, zs = [], [], []
    for v1, v2, v3 in triangles:
        for x, y, z in (v1, v2, v3):
            xs.append(x)
            ys.append(y)
            zs.append(z)
    return min(xs), max(xs), min(ys), max(ys), min(zs), max(zs)


def transform_vertex(x, y, z, cx, cy, cz, scale):
    return (
        round((x - cx) * scale),
        round((y - cy) * scale),
        round((z - cz) * scale),
    )


def face_normal(v0, v1, v2):
    ax, ay, az = v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]
    bx, by, bz = v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]
    nx = ay * bz - az * by
    ny = az * bx - ax * bz
    nz = ax * by - ay * bx
    length = math.sqrt(nx * nx + ny * ny + nz * nz)
    if length == 0:
        return 0, 8192, 0
    s = NORMAL_SCALE / length
    return round(nx * s), round(ny * s), round(nz * s)


def format_face(vertices):
    nx, ny, nz = face_normal(*vertices)
    lines = []
    for x, y, z in vertices:
        lines.append(f"    {{{x},{y},{z}, {nx},{ny},{nz}}},")
    return "  {  " + lines[0] + "\n" + "\n".join(lines[1:]) + "\n  }"


def convert(input_path: Path, output_path: Path):
    triangles = read_stl(input_path)
    oriented = []
    for v1, v2, v3 in triangles:
        oriented.append(
            (
                orient_vertex(*v1),
                orient_vertex(*v2),
                orient_vertex(*v3),
            )
        )
    triangles = oriented
    xmin, xmax, ymin, ymax, zmin, zmax = bounds(triangles)
    cx = (xmin + xmax) / 2
    cy = (ymin + ymax) / 2
    cz = (zmin + zmax) / 2
    extent = max(xmax - xmin, ymax - ymin, zmax - zmin)
    scale = TARGET_SIZE / extent if extent else 1.0

    faces = []
    for v1, v2, v3 in triangles:
        vertices = (
            transform_vertex(*v1, cx, cy, cz, scale),
            transform_vertex(*v2, cx, cy, cz, scale),
            transform_vertex(*v3, cx, cy, cz, scale),
        )
        faces.append(format_face(vertices))

    body = ",\n".join(faces)
    output_path.write_text(
        HEADER.format(source=input_path.name, n_faces=len(faces))
        + body
        + ",\n};\n",
        encoding="utf-8",
    )
    print(f"Wrote {len(faces)} faces to {output_path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_stl", type=Path)
    parser.add_argument("output_h", type=Path)
    args = parser.parse_args()
    convert(args.input_stl, args.output_h)


if __name__ == "__main__":
    main()
