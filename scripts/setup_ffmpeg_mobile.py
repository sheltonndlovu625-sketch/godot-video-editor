#!/usr/bin/env python3
"""
Setup script for mobile FFmpeg libraries.
Downloads prebuilt binaries when available.
If download fails, prints manual build instructions.
"""
import os
import sys
import urllib.request
import zipfile
import shutil


def download(url, dest):
    print(f"Downloading: {url}")
    try:
        urllib.request.urlretrieve(url, dest)
        print(f"Saved: {dest}")
        return True
    except Exception as e:
        print(f"Download failed: {e}")
        return False


def setup_android(ndk_path: str, api: int = 24):
    """Download or prepare FFmpeg for Android."""
    out = "ffmpeg-android"
    os.makedirs(out, exist_ok=True)

    # NOTE: Prebuilt Android FFmpeg is tricky. 
    # This script attempts to download from community builds.
    # If this fails, you must build FFmpeg manually with the NDK.

    aar_url = (
        "https://github.com/tanersener/mobile-ffmpeg/releases/download/v4.4/"
        f"mobile-ffmpeg-full-4.4-android-api{api}.aar"
    )
    aar_path = os.path.join(out, "ffmpeg.aar")

    if download(aar_url, aar_path):
        with zipfile.ZipFile(aar_path, 'r') as z:
            z.extractall(out)
        print("[Android] Extracted AAR. .so files are in ffmpeg-android/jni/")
        print("[Android] You still need FFmpeg headers (include/).")
        print("[Android] Download FFmpeg source and copy the include/ folder here.")
    else:
        print("[Android] Could not download prebuilt FFmpeg.")
        print("[Android] Manual build required. See README.md")


def setup_ios():
    """Download or prepare FFmpeg for iOS."""
    out = "ffmpeg-ios"
    os.makedirs(out, exist_ok=True)

    zip_url = (
        "https://github.com/tanersener/ffmpeg-ios/releases/download/v4.4/"
        "ffmpeg-ios-full-4.4.zip"
    )
    zip_path = os.path.join(out, "ffmpeg-ios.zip")

    if download(zip_url, zip_path):
        with zipfile.ZipFile(zip_path, 'r') as z:
            z.extractall(out)
        print("[iOS] Extracted frameworks.")
    else:
        print("[iOS] Could not download prebuilt FFmpeg.")
        print("[iOS] Manual build required. See README.md")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: setup_ffmpeg_mobile.py <android|ios> [ndk_path]")
        sys.exit(1)

    platform = sys.argv[1]
    if platform == "android":
        ndk = sys.argv[2] if len(sys.argv) > 2 else os.environ.get("ANDROID_NDK_ROOT", "")
        setup_android(ndk, 24)
    elif platform == "ios":
        setup_ios()
    else:
        print(f"Unknown platform: {platform}")
        sys.exit(1)
