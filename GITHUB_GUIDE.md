# GitHub Beginner's Guide

> **Never used GitHub before?** This guide walks you through everything.

## Step 1: Create a GitHub Account

1. Go to [github.com](https://github.com)
2. Click **Sign up**
3. Follow the prompts (email, password, username)
4. Verify your email

## Step 2: Create Your Repository

1. Click the **+** icon (top right) → **New repository**
2. Name it: `godot-video-editor`
3. Choose **Public** (free, visible to everyone) or **Private**
4. **DO NOT** initialize with README (we already have one)
5. Click **Create repository**

## Step 3: Upload This Project

### Option A: GitHub Web Upload (Easiest for Beginners)

1. On your new repo page, click **uploading an existing file**
2. Drag and drop the entire project folder contents
3. Wait for upload, then click **Commit changes**

### Option B: Git Command Line (Recommended for updates)

```bash
# Install Git first: https://git-scm.com/downloads

cd godot-video-editor

# Initialize git
git init

# Add all files
git add .

# Commit
git commit -m "Initial video editor project"

# Connect to GitHub (replace YOUR_USERNAME)
git remote add origin https://github.com/YOUR_USERNAME/godot-video-editor.git

# Push
git branch -M main
git push -u origin main
```

## Step 4: GitHub Actions Builds (Automatic!)

Once you push the project, GitHub Actions **automatically activates** because the file `.github/workflows/build.yml` exists.

### View Build Status

1. Go to your repo on GitHub
2. Click the **Actions** tab (next to Pull requests)
3. You'll see builds running for Linux, Windows, macOS, Android, and iOS
4. Green checkmark = success, Red X = failed

### Download Compiled Libraries

1. Click a completed workflow run
2. Scroll to **Artifacts** at the bottom
3. Download the platform files you need:
   - `linux-libraries`
   - `windows-libraries`
   - `macos-libraries`
   - `android-libraries`
   - `ios-libraries`
4. Unzip and place the files in `godot_project/addons/video_encoder/bin/<platform>/`

## Step 5: Create a Release (Optional)

Releases let you distribute stable versions:

1. Go to **Releases** (right side of repo page)
2. Click **Create a new release**
3. Tag version: `v0.1.0`
4. Title: "Initial Release"
5. Attach your built libraries (drag and drop)
6. Click **Publish release**

## Common Issues

### "godot-cpp not found" in CI

The workflow auto-clones godot-cpp. If it fails, check the **Actions** log for network errors.

### Android/iOS Build Fails

Mobile FFmpeg libraries are tricky. The CI tries to download prebuilt binaries. If this fails:
- Check the **Setup FFmpeg** step logs
- You may need to manually build FFmpeg and upload libraries as repo secrets/assets

### Windows Build Fails

Make sure the MSYS2 step installs all packages. The workflow uses MinGW64.

## Next Steps

1. **Develop locally** in Godot Editor
2. **Push changes** to GitHub: `git add . && git commit -m "update" && git push`
3. **Download fresh builds** from Actions after each push
4. **Iterate!** The CI handles all cross-compilation for you.

## Help

- GitHub Docs: [docs.github.com](https://docs.github.com)
- Godot GDExtension: [docs.godotengine.org](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/index.html)
- FFmpeg Docs: [ffmpeg.org/documentation.html](https://ffmpeg.org/documentation.html)
