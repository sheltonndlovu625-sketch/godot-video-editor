#!/bin/bash
set -euo pipefail

NDK="${ANDROID_NDK_ROOT:-}"
if [ -z "$NDK" ]; then
    echo "ERROR: ANDROID_NDK_ROOT not set"
    exit 1
fi

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

SRC_DIR="$WORKSPACE/ffmpeg-$FFMPEG_VERSION"

COMMON_FLAGS=(
    --target-os=android
    --enable-cross-compile
    --disable-programs
    --disable-doc
    --disable-shared
    --enable-static
    --enable-gpl
    --enable-version3
    --disable-asm
    --disable-x86asm
    --disable-inline-asm
    --disable-stripping
    --disable-bzlib
    --disable-libopenjpeg
    --disable-iconv
    --disable-zlib
    --disable-everything

    --enable-avformat
    --enable-avcodec
    --enable-avutil
    --enable-swscale
    --enable-swresample

    --enable-decoder=h264
    --enable-decoder=hevc
    --enable-decoder=mpeg4
    --enable-decoder=vp8
    --enable-decoder=vp9
    --enable-decoder=mjpeg
    --enable-decoder=png
    --enable-decoder=mp3
    --enable-decoder=aac

    --enable-encoder=mjpeg
    --enable-encoder=aac
    --enable-encoder=png

    --enable-muxer=mp4
    --enable-muxer=mov
    --enable-muxer=image2
    --enable-demuxer=mp4
    --enable-demuxer=mov
    --enable-demuxer=image2

    --enable-protocol=file

    --enable-parser=mjpeg
    --enable-parser=aac
    --enable-parser=h264
    --enable-parser=hevc
)

build_arch() {
    local ARCH=$1
    local CPU=$2
    local CLANG_PREFIX=$3
    local OUT_DIR="$INSTALL_PREFIX/$ARCH"

    echo "=== Building FFmpeg for Android $ARCH ==="
    mkdir -p "$OUT_DIR"

    local BUILD_DIR="$WORKSPACE/build-ffmpeg-android-$ARCH"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    "$SRC_DIR/configure" \
        --prefix="$OUT_DIR" \
        --arch="$ARCH" \
        --cpu="$CPU" \
        --cc="$TOOLCHAIN/bin/${CLANG_PREFIX}${API_LEVEL}-clang" \
        --cxx="$TOOLCHAIN/bin/${CLANG_PREFIX}${API_LEVEL}-clang++" \
        --ar="$TOOLCHAIN/bin/llvm-ar" \
        --as="$TOOLCHAIN/bin/${CLANG_PREFIX}${API_LEVEL}-clang" \
        --strip="$TOOLCHAIN/bin/llvm-strip" \
        --nm="$TOOLCHAIN/bin/llvm-nm" \
        --ranlib="$TOOLCHAIN/bin/llvm-ranlib" \
        --sysroot="$TOOLCHAIN/sysroot" \
        --extra-cflags="-O3 -fPIC" \
        --extra-ldflags="-O3" \
        "${COMMON_FLAGS[@]}" || {
            echo "=== CONFIGURE FAILED for $ARCH ==="
            tail -n 100 "$BUILD_DIR/ffbuild/config.log" 2>/dev/null || echo "no config.log"
            exit 1
        }

    make -j$(nproc)
    make install
}

build_arch "aarch64" "armv8-a" "aarch64-linux-android"
build_arch "arm"     "armv7-a" "armv7a-linux-androideabi"
build_arch "x86_64"  "x86-64"  "x86_64-linux-android"

echo "=== FFmpeg Android build complete ==="
echo "Libraries installed to: $INSTALL_PREFIX"
