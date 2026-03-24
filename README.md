# brickr in Docker

This workspace packages [Daekkyn/brickr](https://github.com/Daekkyn/brickr) in Docker so the Qt/OpenGL app can be built reproducibly on Linux.

## What changed

- Added a `Dockerfile` to build and run the app in Ubuntu 22.04.
- Added `docker-compose.yml` for a one-command local setup.
- Replaced the bundled `binvox` runtime path with a Python voxelizer that works on Apple Silicon.
- Added a browser-based noVNC desktop so the Qt GUI can be used on macOS without XQuartz.
- Fixed BINVOX import dimension handling so generated voxel files load correctly.

## Build

```sh
docker build --platform=linux/amd64 -t brickr:local .
```

## Run

This image starts Brickr in a virtual desktop and exposes it in your browser with noVNC:

```sh
docker run --rm -it --platform=linux/amd64 -p 6080:6080 -p 5900:5900 brickr:local
```

Then open:

```txt
http://localhost:6080/vnc.html
```

To work with your own meshes, mount the repo or a models folder:

```sh
docker run --rm -it \
  --platform=linux/amd64 \
  -p 6080:6080 \
  -p 5900:5900 \
  -v "$PWD/models:/opt/brickr/models" \
  -v "$PWD/output:/opt/brickr/output" \
  brickr:local
```

Or with Compose:

```sh
mkdir -p output
docker compose up --build
```

## Notes

- `brickr` is a GUI app, not a command-line converter.
- Mesh input support is currently `.obj`.
- The bundled `binvox` binary is Linux x86_64, so the container is pinned to `linux/amd64`.
- `BINVOX_PATH` is set automatically to the Python voxelizer inside the image.
- The easiest way to see the GUI on macOS is the built-in noVNC desktop at `http://localhost:6080/vnc.html`.
- In the browser container, exports are written into `/opt/brickr/output`, which maps to local `./output`.
