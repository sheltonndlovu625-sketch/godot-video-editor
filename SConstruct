#!/usr/bin/env python
import os
import sys

if not os.path.exists("godot-cpp"):
    print("ERROR: godot-cpp not found.")
    print("Run: bash scripts/setup_godot_cpp.sh")
    sys.exit(1)

env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

# ------------------------------------------------------------------
# FFmpeg Configuration
# ------------------------------------------------------------------
ffmpeg_path = ARGUMENTS.get("ffmpeg_path", os.environ.get("FFMPEG_PATH", ""))

if ffmpeg_path:
    ffmpeg_arch = ""
    if env["platform"] == "android":
        arch_map = {"arm64": "aarch64", "arm32": "arm", "x86_64": "x86_64"}
        ffmpeg_arch = arch_map.get(env["arch"], env["arch"])
    elif env["platform"] == "ios":
        ffmpeg_arch = env["arch"]

    if ffmpeg_arch:
        ffmpeg_include = os.path.join(ffmpeg_path, ffmpeg_arch, "include")
        ffmpeg_lib = os.path.join(ffmpeg_path, ffmpeg_arch, "lib")
    else:
        ffmpeg_include = os.path.join(ffmpeg_path, "include")
        ffmpeg_lib = os.path.join(ffmpeg_path, "lib")

    if os.path.exists(ffmpeg_include):
        env.Append(CPPPATH=[ffmpeg_include])
        print("FFmpeg include: " + ffmpeg_include)
    else:
        print("WARNING: FFmpeg include not found: " + ffmpeg_include)

    if os.path.exists(ffmpeg_lib):
        env.Append(LIBPATH=[ffmpeg_lib])
        print("FFmpeg lib: " + ffmpeg_lib)
    else:
        print("WARNING: FFmpeg lib not found: " + ffmpeg_lib)

ffmpeg_libs = ["avformat", "avcodec", "swresample", "swscale", "avutil"]

# ------------------------------------------------------------------
# Platform-Specific Tweaks
# ------------------------------------------------------------------
if env["platform"] == "android":
    env.Append(SHLINKFLAGS=["-Wl,-soname,libvideo_encoder.android.{}.{}.so".format(env["target"], env["arch"])])
    env.Append(LIBS=["android", "log"])
    env.Append(LINKFLAGS=[
        "-Wl,--whole-archive",
        "-lavformat", "-lavcodec", "-lswresample", "-lswscale", "-lavutil",
        "-Wl,--no-whole-archive"
    ])

elif env["platform"] == "ios":
    env.Append(CPPDEFINES=["IOS_ENABLED"])
    env.Append(LINKFLAGS=[
        "-framework", "Foundation",
        "-framework", "CoreVideo",
        "-framework", "CoreMedia",
        "-framework", "AVFoundation",
        "-framework", "VideoToolbox",
    ])
    if ffmpeg_path and os.path.exists(ffmpeg_lib):
        for lib in ffmpeg_libs:
            lib_path = os.path.join(ffmpeg_lib, "lib" + lib + ".a")
            if os.path.exists(lib_path):
                env.Append(LINKFLAGS=["-Wl,-force_load," + lib_path])

elif env["platform"] == "windows":
    env.Append(LIBS=ffmpeg_libs)
    env.Append(LIBS=[
        "bcrypt", "strmiids", "uuid", "ole32",
        "ws2_32", "secur32", "mfplat", "mfuuid"
    ])

elif env["platform"] == "linux":
    env.Append(LIBS=ffmpeg_libs)
    env.Append(LIBS=["pthread", "dl", "m"])

elif env["platform"] == "macos":
    env.Append(LIBS=ffmpeg_libs)
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

library = env.SharedLibrary(target=lib_path, source=sources)
Default(library)
