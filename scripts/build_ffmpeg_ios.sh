#!/bin/bash
set -e

FFMPEG_VERSION="6.1"
BUILD_DIR="$(pwd)/ffmpeg-ios"
IOS_DEPLOYMENT_TARGET="13.0"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "=== DOWNLOAD ==="
wget -q --show-progress "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz"
tar -xf "ffmpeg-${FFMPEG_VERSION}.tar.xz"
cd "ffmpeg-${FFMPEG_VERSION}"

SYSROOT=$(xcrun --sdk iphonesimulator --show-sdk-path)
CC="$(xcrun --find clang)"

echo "=== CONFIGURE iOS SIM ==="
./configure \
    --prefix="$BUILD_DIR/build-sim" \
    --target-os=darwin \
    --arch=x86_64 \
    --cpu=x86_64 \
    --enable-cross-compile \
    --sysroot="$SYSROOT" \
    --cc="$CC" \
    --extra-cflags="-arch x86_64 -isysroot $SYSROOT -mios-version-min=$IOS_DEPLOYMENT_TARGET" \
    --extra-ldflags="-arch x86_64 -isysroot $SYSROOT -mios-version-min=$IOS_DEPLOYMENT_TARGET" \
    --disable-programs \
    --disable-doc \
    --disable-network \
    --disable-everything \
    --enable-avcodec \
    --enable-avformat \
    --enable-avutil \
    --enable-swscale \
    --enable-swresample \
    --enable-encoder=h264_videotoolbox \
    --enable-muxer=mp4 \
    --enable-protocol=file \
    --enable-static \
    --disable-shared \
    --enable-pic \
    --disable-asm \
    --disable-stripping 2>&1 | tail -30

echo "=== MAKE ==="
make -j$(sysctl -n hw.ncpu) 2>&1 | tail -10
make install

echo "=== RESULTS ==="
ls -la "$BUILD_DIR/build-sim/include/" 2>/dev/null || true
ls -la "$BUILD_DIR/build-sim/lib/" 2>/dev/null || true

mkdir -p "$BUILD_DIR/include" "$BUILD_DIR/lib"
cp -r "$BUILD_DIR/build-sim/include/"* "$BUILD_DIR/include/" 2>/dev/null || true
cp "$BUILD_DIR/build-sim/lib/"*.a "$BUILD_DIR/lib/" 2>/dev/null || true
ls -la "$BUILD_DIR/include/" || true
ls -la "$BUILD_DIR/lib/" || true
