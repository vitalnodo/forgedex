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
#  Media in RES_FILES and ASSET_FILES goes in uncompressed, as aapt
#  would have it; NO_COMPRESS=.bin:.dat adds extensions to the list.
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

#  An example sits two levels down.  A project of its own, with
#  forgedex as a submodule, names both before sourcing this:
#  ROOT=forgedex, and KEYS beside its own sources, so that its key
#  does not live inside somebody else's checkout.
#
#  Where setup_deps.sh is, said from where the build was started:
#  an example is built from the repository root, by make.
if [ -n "$ROOT" ]; then SETUP="sh $ROOT/setup_deps.sh"; else SETUP="sh setup_deps.sh"; fi
ROOT="${ROOT:-../..}"
KEYS="${KEYS:-$ROOT/keys}"
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
        echo "Error: $(basename "$tool") not found. Run first: $SETUP" >&2
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
    echo "==> Generating a debug signing key in $KEYS/"
    mkdir -p "$KEYS"
    "$APKSIGN" keygen -k "$KEYS/key.pk8" -c "$KEYS/cert.der" -n debug -d 3650
fi

# ── Build ─────────────────────────────────────────────────

rm -rf classes.dex AndroidManifest.xml resources.arsc app.unsigned.apk app.apk lib assets "$NATIVE_LIB"
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
    #  -s after the example's own flags: an -O2 there keeps debug
    #  information, and with it the paths of this checkout and of
    #  zig's headers, so the library — and the APK — would change
    #  with the directory it was built in.
    zig cc -target aarch64-linux-musl -Oz -fPIC -shared -w \
        -fno-builtin -fvisibility=hidden \
        -I"$ROOT/include" $NATIVE_CFLAGS -s \
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

#  The binary manifest is written beside the dex, under the name it
#  has inside the APK, so that `make check` can hash it like the
#  dex.  The source is manifest.asm, so nothing is overwritten.
"$FASMG" "$MANIFEST" AndroidManifest.xml

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

# ── The archive ───────────────────────────────────────────
#
#  Two builds of the same sources give the same APK, byte for byte,
#  so that one can be checked against another.  What went into it
#  already was the same — fasmg and zig write the same bytes for the
#  same input — but zip records more than contents: when each file
#  was made, who owns it, what umask it was made under.  So every
#  entry is first laid out in a directory of its own, under its path
#  in the APK, with one time and one mode, and the archive is made
#  from there: -X leaves out owner and Unix times, -D leaves out
#  entries for directories, which aapt never writes either.  The
#  time is 1980-01-01, the first a zip entry can hold, in UTC so
#  that the zone of the machine does not move it.
#
#  The signer is deterministic too, so the same key gives the same
#  app.apk.

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
out="$(pwd)/app.unsigned.apk"

#  put <file> <path in the APK>
put() {
    mkdir -p "$stage/$(dirname "$2")"
    cp "$1" "$stage/$2"
    chmod 644 "$stage/$2"
    TZ=UTC touch -t 198001010000 "$stage/$2"
}

#  pack [zip options] <path in the APK>...
pack() {
    (cd "$stage" && TZ=UTC zip -X -D -q "$out" "$@")
}

#  resources.arsc goes in stored, and goes in first.  Stored because
#  the platform maps it out of the APK rather than reading it, and
#  from Android 11 refuses to install one that is deflated; first
#  because mapping also wants it four-byte aligned, and the first
#  entry of an archive begins at a known offset, which is alignment
#  by arithmetic rather than by luck.
if [ -n "$RES_ASM" ]; then
    put resources.arsc resources.arsc
    pack -0 resources.arsc
fi

put classes.dex classes.dex
put AndroidManifest.xml AndroidManifest.xml
pack classes.dex AndroidManifest.xml

#  res/ keeps its paths: the table refers to these files by path.
for layout in $LAYOUT_ASM; do
    name="res/layout/$(basename "$layout" .asm).xml"
    put "$name" "$name"
    pack "$name"
done

#  Media goes in stored.  Already compressed, it gains nothing from
#  deflate, and a player opens a raw resource or an asset as a file
#  descriptor into the APK — which a deflated entry cannot give it:
#  "probably compressed", and no playback.
#
#  The list is aapt's own, the extensions it never compresses.  A
#  project stores more by naming them, the way Gradle's noCompress
#  does: NO_COMPRESS=.bin:.dat in its build.sh.
STORED=.jpg:.jpeg:.png:.gif:.wav:.mp2:.mp3:.ogg:.opus:.aac:.mpg:.mpeg
STORED=$STORED:.mid:.midi:.smf:.jet:.rtttl:.imy:.xmf:.mp4:.m4a:.m4v
STORED=$STORED:.3gp:.3gpp:.3g2:.3gpp2:.amr:.awb:.wma:.wmv:.webm:.mkv
STORED=$STORED${NO_COMPRESS:+:$NO_COMPRESS}

if [ -n "$RES_FILES" ]; then
    for f in $RES_FILES; do
        put "$f" "$f"
    done
    # shellcheck disable=SC2086
    pack -n "$STORED" $RES_FILES
fi

if [ -n "$NATIVE_LIB" ]; then
    put "$NATIVE_LIB" "lib/arm64-v8a/$(basename "$NATIVE_LIB")"
    pack "lib/arm64-v8a/$(basename "$NATIVE_LIB")"
fi

#  assets/ is the one part of an APK that needs no aapt and no
#  resource ids: entries go in under their own names, and
#  AssetManager.open finds them by those names.
if [ -n "$ASSET_FILES" ]; then
    for asset in $ASSET_FILES; do
        put "$asset" "assets/$(basename "$asset")"
    done
    pack -n "$STORED" $(for asset in $ASSET_FILES; do echo "assets/$(basename "$asset")"; done)
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
