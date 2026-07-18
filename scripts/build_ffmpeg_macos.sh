#!/bin/bash
set -euxo pipefail

WORKSPACE="${GITHUB_WORKSPACE:-$(pwd)}"
INSTALL_PREFIX="$WORKSPACE/ffmpeg-macos"

# Nuclear clean
rm -rf "$INSTALL_PREFIX"
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
    --enable-pic
    --disable-asm
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

    --enable-decoder=h264_videotoolbox
    --enable-decoder=hevc_videotoolbox

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

build_macos() {
    local ARCH=$1
    local SDK=$2

    echo "=== Building FFmpeg for macOS $ARCH ($SDK) ==="

    local BUILD_DIR="$WORKSPACE/build-ffmpeg-macos-$ARCH"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    local SYSROOT
    SYSROOT=$(xcrun -sdk "$SDK" --show-sdk-path)
    local CC="xcrun -sdk $SDK clang"
    local CXX="xcrun -sdk $SDK clang++"

    local EXTRA_CFLAGS="-arch $ARCH -mmacosx-version-min=11.0 -O3 -fPIC"

    if [ "$ARCH" = "arm64" ]; then
        EXTRA_CFLAGS="$EXTRA_CFLAGS -march=armv8-a"
    fi

    "$SRC_DIR/configure" \
        --prefix="$INSTALL_PREFIX" \
        --arch="$ARCH" \
        --cc="$CC" \
        --cxx="$CXX" \
        --ar="xcrun ar" \
        --strip="xcrun strip" \
        --nm="xcrun nm" \
        --ranlib="xcrun ranlib" \
        --sysroot="$SYSROOT" \
        --extra-cflags="$EXTRA_CFLAGS" \
        --extra-ldflags="-arch $ARCH -mmacosx-version-min=11.0 -O3" \
        "${COMMON_FLAGS[@]}" || {
            echo "=== CONFIGURE FAILED for macOS $ARCH ==="
            tail -n 100 "$BUILD_DIR/ffbuild/config.log" 2>/dev/null || echo "no config.log"
            exit 1
        }

    make -j$(sysctl -n hw.ncpu)
    make install

    cd "$WORKSPACE"
}

build_macos "arm64" "macosx"
build_macos "x86_64" "macosx"

echo "=== FFmpeg macOS build complete ==="
echo "Libraries installed to: $INSTALL_PREFIX"
ls -la "$INSTALL_PREFIX/lib/"
