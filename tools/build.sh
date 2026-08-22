#!/bin/sh
# ══════════════════════════════════════════════════════════
#  The build every example shares.
#
#  An example's own build.sh names its parts and sources this:
#
#      #!/bin/sh
#      cd "$(dirname "$0")"
#      ASM=raycaster.asm
#      PACKAGE=app.hello.ray
#      ACTIVITY=.Main
#      . ../../tools/build.sh
#
#  An example with a native library names two more, and may add
#  compiler flags of its own and files to carry in assets/:
#
#      NATIVE_SRC="dtmf.c shim/shim.c ioccc/prog.c"
#      NATIVE_LIB=libdtmf.so
#      NATIVE_CFLAGS="-I shim -Dmain=dtmf_main"
#      ASSET_FILES=ioccc/dtmf.bin
#
#  An example with resources names those instead:
#
#      RES_ASM=res.asm                     # -> resources.arsc
#      LAYOUT_ASM="screen.asm"             # -> res/layout/screen.xml
#      RES_FILES="res/drawable/icon.png"   # carried in as they are
#
#  The steps are: assemble the dex, assemble the manifest, zip the
#  two together (with lib/arm64-v8a/ when there is a native
#  library), and sign.  Signing keys are made on first use.
#
#  Both assemblies are fasmg, and the signer makes its own key, so
#  the whole build needs fasmg and — for a native library — zig.
#  Nothing else.
# ══════════════════════════════════════════════════════════

set -e

ROOT="../.."
KEYS="$ROOT/keys"
FASMG="$ROOT/deps/fasmg"
APKSIGN="$ROOT/deps/apksign"

: "${ASM:?an example must set ASM}"
: "${PACKAGE:?an example must set PACKAGE}"
: "${ACTIVITY:?an example must set ACTIVITY}"

#  The manifest is assembled like everything else.  An example that
#  needs more than one activity or a permission writes its own;
#  the rest name three values and let tools/manifest.inc do it.

MANIFEST=${MANIFEST:-manifest.asm}

# ── Uninstall asks nothing of the build ───────────────────
#
#  This comes first on purpose.  Reaching it after the build meant
#  assembling and signing an APK on the way to throwing one away.

if [ "$1" = "--uninstall" ]; then
    adb uninstall "$PACKAGE"
    exit 0
fi

# ── Tools ─────────────────────────────────────────────────

for tool in "$FASMG" "$APKSIGN"; do
    if [ ! -f "$tool" ]; then
        echo "Error: $(basename "$tool") not found. Run first: sh setup_deps.sh" >&2
        exit 1
    fi
done

if [ -n "$NATIVE_SRC" ] && ! command -v zig >/dev/null 2>&1; then
    echo "Error: zig not found, and $NATIVE_SRC needs it. See https://ziglang.org/" >&2
    exit 1
fi

# ── Signing keys, made once ───────────────────────────────

if [ ! -f "$KEYS/key.pk8" ]; then
    #  P-256, because apksign signs with Zig's std and Zig's std has
    #  ECDSA and no RSA.  It mints the certificate too, so no openssl
    #  is involved, and the public key is derived rather than stored.
    echo "==> Generating a debug signing key in keys/"
    mkdir -p "$KEYS"
    "$APKSIGN" keygen -k "$KEYS/key.pk8" -c "$KEYS/cert.der" -n debug -d 3650
fi

# ── Build ─────────────────────────────────────────────────

rm -rf classes.dex resources.arsc app.unsigned.apk app.apk lib assets "$NATIVE_LIB"
for layout in $LAYOUT_ASM; do rm -f "res/layout/$(basename "$layout" .asm).xml"; done

#  Freestanding on purpose: the library reaches the outside world
#  only through the JNIEnv function table, so it needs no libc and
#  names musl only because zig wants a target.  Everything is
#  hidden but JNI_OnLoad, so that a library of ours cannot stand in
#  front of some other library's memcpy.
#  A stub is a library the linker can be satisfied by and the
#  loader will never use: the soname the device knows it by, the
#  names being linked against, and nothing behind them.  It is how
#  a platform library is reached without the NDK.  The soname
#  comes from the file's own name.
for stub in $NATIVE_STUBS; do
    soname=$(basename "$stub" .c).so
    zig cc -target aarch64-linux-musl -shared -w \
        -Wl,-soname,"$soname" \
        -o "$(dirname "$stub")/$soname" "$stub"
done

if [ -n "$NATIVE_SRC" ]; then
    # shellcheck disable=SC2086
    zig cc -target aarch64-linux-musl -Oz -fPIC -shared -w \
        -fno-builtin -fvisibility=hidden \
        -I"$ROOT/include" $NATIVE_CFLAGS \
        -o "$NATIVE_LIB" $NATIVE_SRC

    #  What the library asks the loader for is checked against what
    #  the example said it would ask for.  NATIVE_NEEDS unset means
    #  freestanding, and then a stray libc call shows up here as a
    #  DT_NEEDED rather than as a crash on the device.
    if command -v readelf >/dev/null 2>&1; then
        got=$(readelf -d "$NATIVE_LIB" \
              | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' | sort | tr '\n' ' ')
        want=$(for n in $NATIVE_NEEDS; do echo "$n"; done | sort | tr '\n' ' ')
        if [ "$got" != "$want" ]; then
            echo "Error: $NATIVE_LIB asks the loader for: ${got:-nothing}" >&2
            echo "       the example declared:            ${want:-nothing}" >&2
            exit 1
        fi
    fi
fi

"$FASMG" "$ASM" classes.dex

#  The binary manifest has to enter the APK under the same name as
#  the source, so it is written into a directory of its own rather
#  than beside it.  That directory is private to this build: a
#  fixed path under /tmp would collide when two examples build at
#  once, as `make -j` does.
axml=$(mktemp -d)
trap 'rm -rf "$axml"' EXIT
"$FASMG" "$MANIFEST" "$axml/AndroidManifest.xml"

if [ -n "$RES_ASM" ]; then
    "$FASMG" "$RES_ASM" resources.arsc
fi

#  A layout is named after its source: screen.asm becomes
#  res/layout/screen.xml, which is the path the resource table
#  points at.
for layout in $LAYOUT_ASM; do
    mkdir -p res/layout
    "$FASMG" "$layout" "res/layout/$(basename "$layout" .asm).xml"
done

#  resources.arsc goes in stored, and goes in first.  Stored because
#  the platform maps it out of the APK rather than reading it, and
#  from Android 11 refuses to install one that is deflated; first
#  because mapping also wants it four-byte aligned, and the first
#  entry of an archive begins at a known offset, which is alignment
#  by arithmetic rather than by luck.
if [ -n "$RES_ASM" ]; then
    zip -jq0 app.unsigned.apk resources.arsc
fi

zip -jq app.unsigned.apk classes.dex "$axml/AndroidManifest.xml"

#  res/ keeps its paths — `zip -j` is there to throw them away, and
#  the table refers to these files by path.
for layout in $LAYOUT_ASM; do
    zip -q app.unsigned.apk "res/layout/$(basename "$layout" .asm).xml"
done
if [ -n "$RES_FILES" ]; then
    # shellcheck disable=SC2086
    zip -q app.unsigned.apk $RES_FILES
fi

if [ -n "$NATIVE_LIB" ]; then
    mkdir -p lib/arm64-v8a
    cp "$NATIVE_LIB" lib/arm64-v8a/
    zip -rq app.unsigned.apk lib/
fi

#  assets/ is the one part of an APK that needs no aapt and no
#  resource ids: entries go in under their own names, and
#  AssetManager.open finds them by those names.
if [ -n "$ASSET_FILES" ]; then
    mkdir -p assets
    for asset in $ASSET_FILES; do
        cp "$asset" assets/
    done
    zip -rq app.unsigned.apk assets/
fi

#  v1 and v2: v1 is what an old enough device knows, v2 is what API
#  30 and later insists on.
"$APKSIGN" sign \
    -i app.unsigned.apk \
    -o app.apk \
    -k "$KEYS/key.pk8" \
    -c "$KEYS/cert.der" \
    -s v1+v2

#  What went in stored has to still be stored on the way out: a
#  signer that rebuilt the archive would quietly deflate it, and the
#  failure would arrive as a refusal to install rather than as
#  anything about this build.  Scheme v2 wedges its block in and
#  touches no entry, which is why this holds.
if [ -n "$RES_ASM" ] && command -v unzip >/dev/null 2>&1; then
    if ! unzip -v app.apk | grep -q 'Stored.*resources\.arsc'; then
        echo "Error: resources.arsc came out of the signer compressed" >&2
        exit 1
    fi
fi

# ── Install ───────────────────────────────────────────────

if [ "$1" = "--install" ]; then
    adb uninstall "$PACKAGE" 2>/dev/null || true
    adb install app.apk
    adb shell am start -n "$PACKAGE/$ACTIVITY"
else
    echo "Built: app.apk"
    echo "  install:   sh build.sh --install"
    echo "  uninstall: sh build.sh --uninstall"
fi
