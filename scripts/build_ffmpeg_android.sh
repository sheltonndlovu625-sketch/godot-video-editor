#!/bin/bash
set -e

FFMPEG_VERSION="6.1"
BUILD_DIR="$(pwd)/ffmpeg-android"
ANDROID_API=24

ANDROID_NDK=${ANDROID_NDK_ROOT:-$1}
if [ -z "$ANDROID_NDK" ]; then
    echo "ERROR: ANDROID_NDK_ROOT not set"
    exit 1
fi

echo "=== NDK PATH ==="
echo "$ANDROID_NDK"
ls -la "$ANDROID_NDK" 2>/dev/null | head -10 || true

HOST_TAG="linux-x86_64"
TOOLCHAIN="$ANDROID_NDK/toolchains/llvm/prebuilt/$HOST_TAG"

echo "=== TOOLCHAIN ==="
ls -la "$TOOLCHAIN/bin/" 2>/dev/null | grep clang | head -5 || true

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "=== DOWNLOAD ==="
wget -q --show-progress "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz"
tar -xf "ffmpeg-${FFMPEG_VERSION}.tar.xz"
cd "ffmpeg-${FFMPEG_VERSION}"

echo "=== CONFIGURE ARM64 ==="
./configure \
    --prefix="$BUILD_DIR/build-arm64" \
    --target-os=android \
    --arch=aarch64 \
    --cpu=armv8-a \
    --enable-cross-compile \
    --cc="$TOOLCHAIN/bin/aarch64-linux-android${ANDROID_API}-clang" \
    --cxx="$TOOLCHAIN/bin/aarch64-linux-android${ANDROID_API}-clang++" \
    --ar="$TOOLCHAIN/bin/llvm-ar" \
    --sysroot="$TOOLCHAIN/sysroot" \
    --extra-cflags="-O3 -fPIC" \
    --disable-programs \
    --disable-doc \
    --disable-network \
    --disable-everything \
    --enable-avcodec \
    --enable-avformat \
    --enable-avutil \
    --enable-swscale \
    --enable-swresample \
    --enable-encoder=aac \
    --enable-muxer=mp4 \
    --enable-protocol=file \
    --enable-static \
    --disable-shared \
    --enable-pic \
    --disable-asm \
    --disable-debug 2>&1 | tail -30

echo "=== MAKE ==="
make -j$(nproc) 2>&1 | tail -10
make install

echo "=== RESULTS ==="
ls -la "$BUILD_DIR/build-arm64/include/" 2>/dev/null || true
ls -la "$BUILD_DIR/build-arm64/lib/" 2>/dev/null || true

mkdir -p "$BUILD_DIR/include" "$BUILD_DIR/lib"
cp -r "$BUILD_DIR/build-arm64/include/"* "$BUILD_DIR/include/" 2>/dev/null || true
cp "$BUILD_DIR/build-arm64/lib/"*.a "$BUILD_DIR/lib/" 2>/dev/null || true
ls -la "$BUILD_DIR/include/" || true
ls -la "$BUILD_DIR/lib/" || true
