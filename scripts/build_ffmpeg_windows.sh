#!/bin/bash
set -euo pipefail

WORKSPACE="${GITHUB_WORKSPACE:-$(pwd)}"
INSTALL_PREFIX="$WORKSPACE/ffmpeg-windows"
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
    --target-os=mingw32
    --arch=x86_64
    --cc=x86_64-w64-mingw32-gcc
    --cxx=x86_64-w64-mingw32-g++
    --enable-static
    --disable-shared
    --disable-programs
    --disable-doc
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
    --enable-muxer=matroska
    --enable-muxer=avi
    --enable-demuxer=mp4
    --enable-demuxer=mov
    --enable-demuxer=image2
    --enable-demuxer=matroska
    --enable-demuxer=avi

    --enable-protocol=file

    --enable-parser=mjpeg
    --enable-parser=aac
    --enable-parser=h264
    --enable-parser=hevc

    --enable-filter=scale
    --enable-filter=format
    --enable-filter=fps

    --disable-libsoxr
    --extra-cflags="-O3 -march=x86-64-v2"
    --extra-ldflags="-static"
    --extra-libs="-lpthread -lm"
)

echo "=== Building FFmpeg for Windows x86_64 ==="
BUILD_DIR="$WORKSPACE/build-ffmpeg-windows"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

"$SRC_DIR/configure" \
    --prefix="$INSTALL_PREFIX" \
    "${COMMON_FLAGS[@]}" || {
        echo "=== CONFIGURE FAILED for Windows ==="
        tail -n 100 "$BUILD_DIR/ffbuild/config.log" 2>/dev/null || echo "no config.log"
        exit 1
    }

make -j$(nproc 2>/dev/null || echo 4)
make install

cd "$WORKSPACE"
echo "=== FFmpeg Windows build complete ==="
echo "Libraries installed to: $INSTALL_PREFIX"
ls -la "$INSTALL_PREFIX/lib/"
