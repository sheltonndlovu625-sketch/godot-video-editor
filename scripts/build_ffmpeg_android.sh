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

echo "Using NDK: $ANDROID_NDK"

HOST_TAG="linux-x86_64"
TOOLCHAIN="$ANDROID_NDK/toolchains/llvm/prebuilt/$HOST_TAG"

if [ ! -d "$TOOLCHAIN" ]; then
    echo "ERROR: Toolchain not found at $TOOLCHAIN"
    ls -la "$ANDROID_NDK/toolchains/llvm/prebuilt/" || true
    exit 1
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f "ffmpeg-${FFMPEG_VERSION}.tar.xz" ]; then
    echo "Downloading FFmpeg source..."
    wget -q "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz"
fi
tar -xf "ffmpeg-${FFMPEG_VERSION}.tar.xz"
cd "ffmpeg-${FFMPEG_VERSION}"

build_ffmpeg() {
    local ARCH=$1
    local TRIPLE=$2
    local CPU=$3
    local PREFIX=$4
    
    echo "Building FFmpeg for ${ARCH}..."
    
    local CC="$TOOLCHAIN/bin/${TRIPLE}${ANDROID_API}-clang"
    local CXX="$TOOLCHAIN/bin/${TRIPLE}${ANDROID_API}-clang++"
    local AR="$TOOLCHAIN/bin/llvm-ar"
    local STRIP="$TOOLCHAIN/bin/llvm-strip"
    local NM="$TOOLCHAIN/bin/llvm-nm"
    local RANLIB="$TOOLCHAIN/bin/llvm-ranlib"
    
    ./configure \
        --prefix="$PREFIX" \
        --target-os=android \
        --arch="$ARCH" \
        --cpu="$CPU" \
        --enable-cross-compile \
        --cc="$CC" \
        --cxx="$CXX" \
        --ar="$AR" \
        --strip="$STRIP" \
        --nm="$NM" \
        --ranlib="$RANLIB" \
        --sysroot="$TOOLCHAIN/sysroot" \
        --extra-cflags="-O3 -fPIC" \
        --extra-ldflags="-Wl,--no-undefined -Wl,-z,noexecstack" \
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
        --enable-filter=scale \
        --enable-static \
        --disable-shared \
        --enable-pic \
        --disable-asm \
        --disable-stripping \
        --disable-debug
    
    make -j$(nproc)
    make install
    make distclean || true
}

build_ffmpeg "aarch64" "aarch64-linux-android" "armv8-a" "$BUILD_DIR/build-arm64"
build_ffmpeg "arm" "armv7a-linux-androideabi" "armv7-a" "$BUILD_DIR/build-armv7"
build_ffmpeg "x86_64" "x86_64-linux-android" "x86-64" "$BUILD_DIR/build-x86_64"

mkdir -p "$BUILD_DIR/include" "$BUILD_DIR/lib"
cp -r "$BUILD_DIR/build-arm64/include/"* "$BUILD_DIR/include/" 2>/dev/null || true
cp "$BUILD_DIR/build-arm64/lib/"*.a "$BUILD_DIR/lib/" 2>/dev/null || true
cp "$BUILD_DIR/build-armv7/lib/"*.a "$BUILD_DIR/lib/" 2>/dev/null || true
cp "$BUILD_DIR/build-x86_64/lib/"*.a "$BUILD_DIR/lib/" 2>/dev/null || true

echo "Android FFmpeg build complete!"
ls -la "$BUILD_DIR/include/"
ls -la "$BUILD_DIR/lib/"
