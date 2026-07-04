#!/bin/bash
set -euo pipefail

NDK="${ANDROID_NDK_ROOT:-/usr/local/lib/android/sdk/ndk/26.1.1090913}"
TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64"
API_LEVEL=24

WORKSPACE="${GITHUB_WORKSPACE:-$(pwd)}"
INSTALL_PREFIX="$WORKSPACE/ffmpeg-android"
mkdir -p "$INSTALL_PREFIX"

FFMPEG_VERSION="6.1.1"
FFMPEG_TAR="ffmpeg-$FFMPEG_VERSION.tar.xz"
FFMPEG_URL="https://ffmpeg.org/releases/$FFMPEG_TAR"

if [ ! -d "ffmpeg-$FFMPEG_VERSION" ]; then
    echo "Downloading FFmpeg $FFMPEG_VERSION..."
    wget -q "$FFMPEG_URL" || curl -L -o "$FFMPEG_TAR" "$FFMPEG_URL"
    tar -xf "$FFMPEG_TAR"
fi

cd "ffmpeg-$FFMPEG_VERSION"

COMMON_FLAGS=(
    --disable-programs
    --disable-doc
    --disable-shared
    --enable-static
    --enable-gpl
    --enable-version3
    --disable-encoders
    --disable-muxers
    --disable-filters
    --disable-devices
    --disable-postproc
    --disable-avdevice
    --disable-network
    --enable-encoder=mjpeg
    --enable-encoder=png
    --enable-decoder=h264
    --enable-decoder=hevc
    --enable-decoder=mpeg4
    --enable-decoder=vp8
    --enable-decoder=vp9
    --enable-decoder=av1
)

build_arch() {
    local ARCH=$1
    local CPU=$2
    local TRIPLE=$3
    local CLANG_PREFIX=$4

    echo "=== Building FFmpeg for Android $ARCH ==="

    local OUT_DIR="$INSTALL_PREFIX/$ARCH"
    mkdir -p "$OUT_DIR"

    ./configure \
        --prefix="$OUT_DIR" \
        --target-os=android \
        --arch="$ARCH" \
        --cpu="$CPU" \
        --enable-cross-compile \
        --cc="$TOOLCHAIN/bin/${CLANG_PREFIX}${API_LEVEL}-clang" \
        --cxx="$TOOLCHAIN/bin/${CLANG_PREFIX}${API_LEVEL}-clang++" \
        --ar="$TOOLCHAIN/bin/llvm-ar" \
        --strip="$TOOLCHAIN/bin/llvm-strip" \
        --nm="$TOOLCHAIN/bin/llvm-nm" \
        --ranlib="$TOOLCHAIN/bin/llvm-ranlib" \
        --cross-prefix="$TOOLCHAIN/bin/${TRIPLE}-" \
        --sysroot="$TOOLCHAIN/sysroot" \
        --extra-cflags="-O3 -fPIC" \
        --extra-ldflags="-O3" \
        "${COMMON_FLAGS[@]}"

    make -j$(nproc)
    make install
    make distclean || true
}

build_arch "aarch64" "armv8-a" "aarch64-linux-android" "aarch64-linux-android"
build_arch "arm"     "armv7-a" "arm-linux-androideabi" "armv7a-linux-androideabi"
build_arch "x86_64"  "x86-64"  "x86_64-linux-android"  "x86_64-linux-android"

echo "=== FFmpeg Android build complete ==="
echo "Libraries installed to: $INSTALL_PREFIX"
