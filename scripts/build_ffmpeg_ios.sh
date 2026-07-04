#!/bin/bash
set -e

FFMPEG_VERSION="6.1"
BUILD_DIR="$(pwd)/ffmpeg-ios"
IOS_DEPLOYMENT_TARGET="13.0"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Download FFmpeg source
if [ ! -f "ffmpeg-${FFMPEG_VERSION}.tar.xz" ]; then
    wget -q "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz"
fi
tar -xf "ffmpeg-${FFMPEG_VERSION}.tar.xz"
cd "ffmpeg-${FFMPEG_VERSION}"

build_ffmpeg() {
    local ARCH=$1
    local SDK=$2
    local PREFIX=$3
    
    echo "Building FFmpeg for ${ARCH} (${SDK})..."
    
    local SYSROOT=$(xcrun --sdk ${SDK} --show-sdk-path)
    local CC="$(xcrun --find clang)"
    
    ./configure \
        --prefix="$PREFIX" \
        --target-os=darwin \
        --arch="$ARCH" \
        --cpu="$ARCH" \
        --enable-cross-compile \
        --sysroot="$SYSROOT" \
        --cc="$CC" \
        --extra-cflags="-arch $ARCH -isysroot $SYSROOT -mios-version-min=$IOS_DEPLOYMENT_TARGET" \
        --extra-ldflags="-arch $ARCH -isysroot $SYSROOT -mios-version-min=$IOS_DEPLOYMENT_TARGET" \
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
        --enable-encoder=aac \
        --enable-muxer=mp4 \
        --enable-protocol=file \
        --enable-filter=scale \
        --enable-static \
        --disable-shared \
        --enable-pic \
        --disable-asm \
        --disable-stripping
    
    make -j$(sysctl -n hw.ncpu)
    make install
    make distclean || true
}

# Build for iOS Simulator x86_64
build_ffmpeg "x86_64" "iphonesimulator" "$BUILD_DIR/build-sim"

# Build for iOS Device ARM64
build_ffmpeg "arm64" "iphoneos" "$BUILD_DIR/build-device"

# Create unified structure
mkdir -p "$BUILD_DIR/include" "$BUILD_DIR/lib"
cp -r "$BUILD_DIR/build-sim/include/"* "$BUILD_DIR/include/" 2>/dev/null || true
cp "$BUILD_DIR/build-sim/lib/"*.a "$BUILD_DIR/lib/" 2>/dev/null || true
cp "$BUILD_DIR/build-device/lib/"*.a "$BUILD_DIR/lib/" 2>/dev/null || true

echo "iOS FFmpeg build complete!"
ls -la "$BUILD_DIR/include/"
ls -la "$BUILD_DIR/lib/"
