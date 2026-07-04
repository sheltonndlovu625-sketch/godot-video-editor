# Godot Video Editor (Starter Project)

A cross-platform video editor built in **Godot 4.4.1** with **FFmpeg**-powered MP4 export via GDExtension.

## Features

- **Native Godot VideoStreamPlayer** for playback (no custom decoder)
- **FFmpeg H.264 encoder** for MP4 export via GDExtension
- **TikTok-style 9:16 aspect-fit** with shader-based effects
- **CameraServer integration** scaffold for live capture
- **Cross-platform builds** via GitHub Actions:
  - Android (ARM64, ARMv7, x86_64)
  - iOS (ARM64, Simulator)
  - Windows (x86_64)
  - macOS (Intel + Apple Silicon)
  - Linux (x86_64)

## Project Structure

```
godot-video-editor/
├── .github/workflows/build.yml   # CI builds for all platforms
├── scripts/
│   ├── setup_godot_cpp.sh        # Fetch godot-cpp submodule
│   └── setup_ffmpeg_mobile.py    # Download mobile FFmpeg libs
├── src/                           # GDExtension C++ source
│   ├── video_encoder.cpp/h        # FFmpeg MP4 encoder
│   └── register_types.cpp/h       # Godot module registration
├── SConstruct                     # Cross-platform build script
└── godot_project/                 # Godot project files
    ├── main.tscn                  # Main editor scene
    ├── main.gd                    # Editor logic
    ├── shaders/                   # Aspect-fit + effect shaders
    └── addons/video_encoder/      # GDExtension config + binaries
```

## Quick Start (Local Development)

### 1. Prerequisites

- **Godot 4.4.1** or later
- **Python 3.11+**
- **SCons**: `pip install scons`
- **FFmpeg dev libraries**:
  - **Linux**: `sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev`
  - **macOS**: `brew install ffmpeg`
  - **Windows**: Use MSYS2 (`pacman -S mingw-w64-x86_64-ffmpeg`) or vcpkg

### 2. Setup

```bash
# Clone this repo
cd godot-video-editor

# Fetch godot-cpp (required for building)
bash scripts/setup_godot_cpp.sh

# Build the GDExtension (Linux example)
scons platform=linux target=template_debug arch=x86_64
scons platform=linux target=template_release arch=x86_64
```

### 3. Open in Godot

1. Open the Godot Editor
2. Import `godot_project/project.godot`
3. The addon should auto-load the compiled library from `addons/video_encoder/bin/`

### 4. Run

- Click **Import** to load a video
- Click **Play/Pause** to preview
- Click **Export MP4** to render using FFmpeg

## GitHub Actions (Automated Builds)

This project includes a complete CI workflow that compiles the GDExtension for **all platforms** automatically.

See [GITHUB_GUIDE.md](GITHUB_GUIDE.md) for step-by-step instructions on using GitHub (for beginners).

### Build Triggers

The workflow runs on:
- Every push to `main` or `master`
- Every pull request
- Manual trigger (workflow_dispatch)

### Artifacts

After each build, compiled libraries are available as downloadable artifacts:
- `linux-libraries`
- `windows-libraries`
- `macos-libraries`
- `android-libraries`
- `ios-libraries`

## Mobile FFmpeg Setup

Android and iOS require prebuilt FFmpeg libraries. The CI attempts to download them automatically via `scripts/setup_ffmpeg_mobile.py`.

If automatic download fails:
1. Build FFmpeg for your target platform using the Android NDK or Xcode toolchain
2. Place headers in `ffmpeg-android/include/` or `ffmpeg-ios/include/`
3. Place libraries in `ffmpeg-android/lib/` or `ffmpeg-ios/lib/`
4. Re-run the build with `FFMPEG_PATH` pointing to your directory

## Architecture Notes

### Why VideoStreamPlayer + SubViewport?

Godot's `VideoStreamPlayer` handles all decoding, audio sync, and format support natively. We route its output through a `SubViewport` so we can:
1. Apply **post-processing shaders** (effects, transitions)
2. Capture frames at a **fixed resolution** for export
3. Maintain **aspect ratio** independently of window size

### FFmpeg Encoder

The GDExtension wraps FFmpeg's `libavcodec`/`libavformat` to encode raw RGBA frames (captured from the SubViewport) into H.264 MP4 files.

## License

MIT License - Starter project scaffold.
