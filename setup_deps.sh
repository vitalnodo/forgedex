#!/bin/sh
# Downloads and builds all dependencies into deps/
# Run once before building examples: sh setup_deps.sh
#
# Installs:
#   deps/fasmg     — flat assembler g (built with system fasm)
#   deps/marco     — AXML converter   (requires Nim)
#   deps/apksigner — APK signer       (requires Go)
#
# To uninstall: rm -rf deps/

set -e

DEPS="$(cd "$(dirname "$0")" && pwd)/deps"
mkdir -p "$DEPS"

# ── fasmg ─────────────────────────────────────────────────
FASMG_REPO="https://github.com/tgrysztar/fasmg"
FASMG_COMMIT="0c48c8c936d6efbeb9c3f1039c7459c8768bbeb1"

if [ ! -f "$DEPS/fasmg" ]; then
    echo "==> Building fasmg ($FASMG_COMMIT)..."
    if ! command -v fasm > /dev/null 2>&1; then
        echo "Error: 'fasm' not found."
        echo "  apt install fasm"
        exit 1
    fi
    git clone "$FASMG_REPO" "$DEPS/fasmg-src"
    git -C "$DEPS/fasmg-src" checkout "$FASMG_COMMIT"
    fasm -m 65536 "$DEPS/fasmg-src/core/source/linux/x64/fasmg.asm" "$DEPS/fasmg"
    rm -rf "$DEPS/fasmg-src"
    echo "    -> deps/fasmg"
else
    echo "    fasmg already built, skipping"
fi

# ── apksigner (Go) ────────────────────────────────────────
if [ ! -f "$DEPS/apksigner" ]; then
    echo "==> Building apksigner..."
    if ! command -v go > /dev/null 2>&1; then
        echo "Error: 'go' not found. Install Go: https://go.dev/"
        exit 1
    fi
    git clone https://github.com/akavel/apksigner "$DEPS/apksigner-src"
    cd "$DEPS/apksigner-src"
    go build -o ../apksigner
    cd - > /dev/null
    rm -rf "$DEPS/apksigner-src"
    echo "    -> deps/apksigner"
else
    echo "    apksigner already built, skipping"
fi

# ── marco (Nim) ───────────────────────────────────────────
if [ ! -f "$DEPS/marco" ]; then
    echo "==> Building marco..."
    if ! command -v nimble > /dev/null 2>&1; then
        echo "Error: 'nimble' not found. Install Nim: https://nim-lang.org/"
        exit 1
    fi
    git clone https://github.com/akavel/marco "$DEPS/marco-src"
    cd "$DEPS/marco-src"
    nimble build -y
    cp marco "$DEPS/marco"
    cd - > /dev/null
    rm -rf "$DEPS/marco-src"
    echo "    -> deps/marco"
else
    echo "    marco already built, skipping"
fi

echo ""
chmod +x "$DEPS/fasmg" "$DEPS/apksigner" "$DEPS/marco"

echo "Done. deps/ contains: fasmg, apksigner, marco"
echo "To remove: rm -rf deps/"