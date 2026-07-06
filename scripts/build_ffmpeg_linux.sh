#!/bin/bash
set -euo pipefail

WORKSPACE="${GITHUB_WORKSPACE:-$(pwd)}"
INSTALL_PREFIX="$WORKSPACE/ffmpeg-linux"
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
    --disable-programs
    --disable-doc
    --disable-shared
    --enable-static
    --enable-pic
    --enable-gpl
    --enable-version3
    --disable-stripping
    --disable-bzlib
    --disable-libopenjpeg
    --disable-iconv
    --disable-zlib
    --disable-everything
    --disable-asm
    --disable-x86asm
    --disable-inline-asm

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

echo "=== Building FFmpeg for Linux x86_64 ==="
BUILD_DIR="$WORKSPACE/build-ffmpeg-linux"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

export CFLAGS="-O3 -fPIC"
export CXXFLAGS="-O3 -fPIC"
export LDFLAGS="-O3 -fPIC"

"$SRC_DIR/configure" \
    --prefix="$INSTALL_PREFIX" \
    --extra-cflags="-O3 -fPIC" \
    --extra-ldflags="-O3 -fPIC" \
    --optflags="-O3 -fPIC" \
    "${COMMON_FLAGS[@]}" || {
        echo "=== CONFIGURE FAILED for Linux ==="
        tail -n 100 "$BUILD_DIR/ffbuild/config.log" 2>/dev/null || echo "no config.log"
        exit 1
    }

make -j$(nproc)
make install

cd "$WORKSPACE"
echo "=== FFmpeg Linux build complete ==="
echo "Libraries installed to: $INSTALL_PREFIX"
ls -la "$INSTALL_PREFIX/lib/"
