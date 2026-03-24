#!/usr/bin/env python3
import argparse
import math
from pathlib import Path

import numpy as np
import trimesh


def write_binvox(path: Path, occupancy: np.ndarray) -> None:
    # Brickr reads dims as depth, height, width and maps the flattened stream as:
    # level = i % height
    # y     = (i / height) % width
    # x     = i / (width * height)
    # So the on-disk order must be [x][y][level] with level varying fastest.
    depth, width, height = occupancy.shape

    data = occupancy.astype(np.uint8).reshape(-1)
    encoded = bytearray()

    if data.size == 0:
        raise ValueError("empty voxel grid")

    current = int(data[0])
    count = 0
    for value in data:
      value = int(value)
      if value == current and count < 255:
        count += 1
        continue

      encoded.extend((current, count))
      current = value
      count = 1

    encoded.extend((current, count))

    with path.open("wb") as f:
      f.write(b"#binvox 1\n")
      f.write(f"dim {depth} {height} {width}\n".encode("ascii"))
      f.write(b"translate 0.0 0.0 0.0\n")
      f.write(b"scale 1.0\n")
      f.write(b"data\n")
      f.write(encoded)


def voxelize(mesh_path: Path, output_path: Path, resolution: int) -> None:
    mesh = trimesh.load(mesh_path, force="mesh")
    if mesh.is_empty:
      raise ValueError(f"failed to load mesh: {mesh_path}")

    bounds = mesh.bounds
    extents = bounds[1] - bounds[0]
    max_extent = float(np.max(extents))
    if max_extent <= 0:
      raise ValueError("mesh has zero size")

    pitch = max_extent / float(resolution)
    vox = mesh.voxelized(pitch=pitch)

    try:
      vox = vox.fill()
    except BaseException:
      pass

    points = np.asarray(vox.points)
    if points.size == 0:
      raise ValueError("voxelization produced no filled cells")

    mins = points.min(axis=0)
    scaled = np.rint((points - mins) / pitch).astype(int)

    dims_xyz = scaled.max(axis=0) + 1
    # Store as [x][y][level] so the flattened stream matches Brickr's parser.
    grid = np.zeros((int(dims_xyz[0]), int(dims_xyz[2]), int(dims_xyz[1])), dtype=np.uint8)

    for x, y, z in scaled:
      grid[x, z, y] = 1

    write_binvox(output_path, grid)


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert OBJ mesh to binvox for Brickr")
    parser.add_argument("-d", "--dimension", type=int, required=True, help="voxel resolution")
    parser.add_argument("input", help="input OBJ path")
    parser.add_argument("output", help="output BINVOX path")
    args = parser.parse_args()

    voxelize(Path(args.input), Path(args.output), args.dimension)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
