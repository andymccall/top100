# Docker build environments

This folder provides containerized build environments to make Linux and Haiku cross-compiles reproducible.

## Images

- base-dev (Dockerfile.base-dev)
  - Ubuntu 24.04 with: gcc/clang toolchain (via build-essential), CMake, Ninja, pkg-config, OpenSSL, libcurl, Boost.
  - Use for core library, CLI, and tests.

- haiku-cross (Dockerfile.haiku-cross)
  - Ubuntu 24.04 with prerequisites plus a helper script to fetch/build Haiku buildtools and cross toolchain.
  - Use for building the Haiku UI target (compile-only inside the container). Running the app still requires Haiku OS.

## Quick start (local)

Build images:

```bash
# From repo root
docker build -f docker/Dockerfile.base-dev -t top100/base-dev:latest .
docker build -f docker/Dockerfile.haiku-cross -t top100/haiku-cross:latest docker
```

Base dev usage:

```bash
docker run --rm -it -v "$PWD":/work -w /work top100/base-dev:latest \
  bash -lc 'cmake -S . -B build -G Ninja -DTOP100_ENABLE_TESTS=ON && cmake --build build -j && ctest --test-dir build --output-on-failure'
```

Haiku cross toolchain bootstrap (one-time per container):

```bash
docker run --rm -it -v "$PWD":/work -w /work top100/haiku-cross:latest \
  bash -lc 'haiku-buildtools.sh'
```

Configure a Haiku cross build:

```bash
docker run --rm -it -v "$PWD":/work -w /work top100/haiku-cross:latest \
  bash -lc 'cmake -S . -B build-haiku -G Ninja -DTOP100_UI_HAIKU=ON -DCMAKE_TOOLCHAIN_FILE=docker/haiku-toolchain.cmake && cmake --build build-haiku -j'
```

Notes:
- The sysroot path in `haiku-toolchain.cmake` may need adjustment depending on the Haiku build output.
- The container cannot run the Haiku binary; copy artifacts to a Haiku system to test.
- For Qt/KDE/GTK Linux UIs, consider a separate `ui-linux` image with Qt/gtkmm/KF installed to avoid bloating `base-dev`.
