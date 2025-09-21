#!/usr/bin/env bash
set -euo pipefail

# This script bootstraps the Haiku cross toolchain and sysroot.
# It aims for a quickstart; for fine control see Haiku docs:
# https://www.haiku-os.org/guides/building/cross-compiling/

# Configurable env vars (defaults):
: "${HAIKU_ARCH:=x86_64}"
: "${HAIKU_GIT:=https://review.haiku-os.org/haiku.git}"
: "${HAIKU_BUILDTOOLS_GIT:=https://review.haiku-os.org/buildtools.git}"
: "${PREFIX:=/opt/haiku}"

mkdir -p "$PREFIX" && cd "$PREFIX"

if [ ! -d buildtools ]; then
  git clone --depth=1 "$HAIKU_BUILDTOOLS_GIT" buildtools
fi
if [ ! -d haiku ]; then
  git clone --depth=1 "$HAIKU_GIT" haiku
fi

# Build buildtools (host tools)
cd "$PREFIX/buildtools"
make -j"$(nproc)" || true

# Build cross tools
cd "$PREFIX/haiku"
set +e
bash ./configure --build-cross-tools $HAIKU_ARCH \
  --cross-tools-prefix "$PREFIX/cross-tools-$HAIKU_ARCH" \
  --build-cross-tools-gcc2 no \
  --target-arch $HAIKU_ARCH
set -e

# Result:
# - Cross tools under $PREFIX/cross-tools-$HAIKU_ARCH/bin
# - Sysroot under $PREFIX/haiku/generated/objects/haiku/$HAIKU_ARCH/packaging/repositories/Haiku/nightly/system

echo "Haiku cross tools ready under $PREFIX/cross-tools-$HAIKU_ARCH/bin"
