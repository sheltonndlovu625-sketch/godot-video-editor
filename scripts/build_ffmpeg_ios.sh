#!/bin/bash
set -euo pipefail

WORKSPACE="${GITHUB_WORKSPACE:-$(pwd)}"
INSTALL_PREFIX="$WORKSPACE/ffmpeg-ios"
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
    --target-os=darwin
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
    --enable-decoder=h264
    --enable-decoder=hevc
    --enable-decoder=mpeg4
    --enable-decoder=vp8
    --enable-decoder=vp9
    --enable-decoder=mp3
    --enable-decoder=aac \
    --enable-encoder=mjpeg
    --enable-encoder=aac \
    --enable-swresample \
    --enable-protocol=file \
    --enable-encoder=aac \
    --enable-swresample \
    --enable-protocol=file \
    --enable-swresample \
    --enable-protocol=file \
    --enable-encoder=png
)

build_ios() {
    local ARCH=$1
    local SDK=$2
    local OUT_DIR="$INSTALL_PREFIX/$ARCH"

    echo "=== Building FFmpeg for iOS $ARCH ($SDK) ==="
    mkdir -p "$OUT_DIR"

    local BUILD_DIR="$WORKSPACE/build-ffmpeg-ios-$ARCH"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    local SYSROOT
    SYSROOT=$(xcrun -sdk "$SDK" --show-sdk-path)
    local CC="xcrun -sdk $SDK clang"
    local CXX="xcrun -sdk $SDK clang++"

    "$SRC_DIR/configure" \
        --prefix="$OUT_DIR" \
        --arch="$ARCH" \
        --cc="$CC" \
        --cxx="$CXX" \
        --ar="xcrun ar" \
        --strip="xcrun strip" \
        --nm="xcrun nm" \
        --ranlib="xcrun ranlib" \
        --sysroot="$SYSROOT" \
        --extra-cflags="-arch $ARCH -mios-version-min=12.0 -O3 -fPIC" \
        --extra-ldflags="-arch $ARCH -mios-version-min=12.0 -O3" \
        "${COMMON_FLAGS[@]}" || {
            echo "=== CONFIGURE FAILED for iOS $ARCH ==="
            tail -n 100 "$BUILD_DIR/ffbuild/config.log" 2>/dev/null || echo "no config.log"
            exit 1
        }

    make -j$(sysctl -n hw.ncpu)
    make install
}

build_ios "arm64" "iphoneos"
build_ios "x86_64" "iphonesimulator"

echo "=== FFmpeg iOS build complete ==="
echo "Libraries installed to: $INSTALL_PREFIX"
