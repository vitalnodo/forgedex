#!/bin/sh
# Downloads and builds all dependencies into deps/
# Run once before building examples: sh setup_deps.sh
#
# Installs:
#   deps/fasmg   — flat assembler g (built with system fasm)
#   deps/apksign — APK signer       (requires Zig)
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

# ── apksign (Zig) ─────────────────────────────────────────
#
#  Signs with schemes v1 and v2 and mints its own key, which is
#  what marco (Nim, and a patch to it) and apksigner (Go, v1 only)
#  and openssl were all here for.
APKSIGN_REPO="https://github.com/vitalnodo/apksign"
APKSIGN_COMMIT="4bcd696"

if [ ! -f "$DEPS/apksign" ]; then
    echo "==> Building apksign ($APKSIGN_COMMIT)..."
    if ! command -v zig > /dev/null 2>&1; then
        echo "Error: 'zig' not found. Install Zig: https://ziglang.org/"
        exit 1
    fi
    git clone "$APKSIGN_REPO" "$DEPS/apksign-src"
    git -C "$DEPS/apksign-src" checkout "$APKSIGN_COMMIT"
    (cd "$DEPS/apksign-src" && zig build -Doptimize=ReleaseSafe)
    cp "$DEPS/apksign-src/zig-out/bin/apksign" "$DEPS/apksign"
    rm -rf "$DEPS/apksign-src"
    echo "    -> deps/apksign"
else
    echo "    apksign already built, skipping"
fi

echo ""
chmod +x "$DEPS/fasmg" "$DEPS/apksign"

echo "Done. deps/ contains: fasmg, apksign"
echo "To remove: rm -rf deps/"