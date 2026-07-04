#!/usr/bin/env python
import os
import sys

# Ensure godot-cpp is present
if not os.path.exists("godot-cpp"):
    print("ERROR: godot-cpp not found.")
    print("Run: bash scripts/setup_godot_cpp.sh")
    sys.exit(1)

# Load godot-cpp build environment
env = SConscript("godot-cpp/SConstruct")

# Add our source files
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

# ------------------------------------------------------------------
# FFmpeg Configuration
# ------------------------------------------------------------------
ffmpeg_path = ARGUMENTS.get("ffmpeg_path", os.environ.get("FFMPEG_PATH", ""))

if ffmpeg_path:
    ffmpeg_include = os.path.join(ffmpeg_path, "include")
    ffmpeg_lib = os.path.join(ffmpeg_path, "lib")

    if os.path.exists(ffmpeg_include):
        env.Append(CPPPATH=[ffmpeg_include])
    if os.path.exists(ffmpeg_lib):
        env.Append(LIBPATH=[ffmpeg_lib])

# FFmpeg libraries to link
# Order matters for static linking: codec -> format -> util -> scale -> resample
ffmpeg_libs = ["avcodec", "avformat", "avutil", "swscale", "swresample"]
env.Append(LIBS=ffmpeg_libs)

# ------------------------------------------------------------------
# Platform-Specific Tweaks
# ------------------------------------------------------------------
if env["platform"] == "android":
    # CRITICAL: Force soname without version suffix for Android .so files
    env.Append(SHLINKFLAGS=["-Wl,-soname,libvideo_encoder.so"])
    env.Append(LIBS=["android", "log"])

elif env["platform"] == "ios":
    env.Append(CPPDEFINES=["IOS_ENABLED"])
    env.Append(LINKFLAGS=[
        "-framework", "Foundation",
        "-framework", "CoreVideo",
        "-framework", "CoreMedia",
        "-framework", "AVFoundation",
    ])

elif env["platform"] == "windows":
    # Windows-specific system libs required by FFmpeg
    env.Append(LIBS=[
        "bcrypt", "strmiids", "uuid", "ole32",
        "ws2_32", "secur32", "mfplat", "mfuuid"
    ])

elif env["platform"] == "linux":
    env.Append(LIBS=["pthread", "dl", "m"])

elif env["platform"] == "macos":
    env.Append(LINKFLAGS=[
        "-framework", "Foundation",
        "-framework", "CoreVideo",
        "-framework", "CoreMedia",
        "-framework", "AVFoundation",
    ])

# ------------------------------------------------------------------
# Output Library Path
# ------------------------------------------------------------------
platform = env["platform"]
target = env["target"]
suffix = env["suffix"]
shlib_suffix = env["SHLIBSUFFIX"]

lib_name = "libvideo_encoder.{}{}".format(platform, suffix)

if platform == "windows":
    lib_name += ".dll"
elif platform == "macos" or platform == "ios":
    lib_name += ".dylib"
elif platform == "android" or platform == "linux":
    lib_name += ".so"

lib_path = "godot_project/addons/video_encoder/bin/{}/{}".format(platform, lib_name)

# Ensure output directory exists
env.Dir(os.path.dirname(lib_path))

# Build the library
library = env.SharedLibrary(target=lib_path, source=sources)
Default(library)
