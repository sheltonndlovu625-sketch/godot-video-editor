#!/bin/bash
set -e
cd "$(dirname "$0")/.."

# ------------------------------------------------------------------
# 1. Init/Update ALL submodules (godot-cpp + whisper.cpp)
# ------------------------------------------------------------------
echo "Updating submodules..."
git submodule update --init --recursive

# ------------------------------------------------------------------
# 2. Pin whisper.cpp to a known-good version (optional but recommended)
# ------------------------------------------------------------------
WHISPER_DIR="whisper.cpp"
WHISPER_TAG="v1.6.2"

if [ -d "$WHISPER_DIR/.git" ]; then
    echo "Pinning $WHISPER_DIR to $WHISPER_TAG..."
    cd "$WHISPER_DIR"
    git fetch --tags --depth 1 origin "$WHISPER_TAG"
    git checkout "$WHISPER_TAG"
    cd ..
fi

# ------------------------------------------------------------------
# 3. Download model weights if missing (skips if already present)
# ------------------------------------------------------------------
MODEL_DIR="godot_project/models"
MODEL_NAME="ggml-base.en.bin"
MODEL_URL="https://huggingface.co/ggerganov/whisper.cpp/resolve/main/$MODEL_NAME"

mkdir -p "$MODEL_DIR"

if [ ! -f "$MODEL_DIR/$MODEL_NAME" ]; then
    echo "Downloading $MODEL_NAME (~142 MB)..."
    if command -v curl >/dev/null 2>&1; then
        curl -L -o "$MODEL_DIR/$MODEL_NAME" "$MODEL_URL"
    elif command -v wget >/dev/null 2>&1; then
        wget -O "$MODEL_DIR/$MODEL_NAME" "$MODEL_URL"
    else
        echo "WARNING: curl/wget not found. Download manually:"
        echo "  $MODEL_URL"
        echo "  -> $MODEL_DIR/$MODEL_NAME"
    fi
else
    echo "Model $MODEL_NAME already present."
fi

echo "Setup complete."
