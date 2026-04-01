#!/bin/sh
# Build counter_plus_minus example
# Run from repo root: sh examples/counter_plus_minus/build.sh
# Or from this directory: sh build.sh

cd "$(dirname "$0")"

KEYS="../../keys"
FASMG="../../deps/fasmg"
MARCO="../../deps/marco"
APKSIGNER="../../deps/apksigner"
PACKAGE="app.hello.counter_plus_minus"

if [ ! -f "$FASMG" ] || [ ! -f "$MARCO" ] || [ ! -f "$APKSIGNER" ]; then
    echo "Error: deps not found. Run first: sh ../../setup_deps.sh"
    exit 1
fi

if [ "${1}" = "--uninstall" ]; then
    adb uninstall "$PACKAGE"
    exit 0
fi

if [ ! -f "$KEYS/key.pk8" ]; then
    mkdir -p "$KEYS"
    openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out "$KEYS/key.pem"
    openssl pkcs8 -topk8 -inform PEM -outform DER -nocrypt -in "$KEYS/key.pem" -out "$KEYS/key.pk8"
    openssl req -new -x509 -key "$KEYS/key.pem" -out "$KEYS/cert.x509.pem" -days 3650 -subj "/CN=debug"
fi

rm -f classes.dex app.unsigned.apk app.apk

"$FASMG" counter_plus_minus.asm classes.dex || exit 1

"$MARCO" < AndroidManifest.xml > /tmp/AndroidManifest.xml || exit 1

zip -j app.unsigned.apk classes.dex /tmp/AndroidManifest.xml

"$APKSIGNER" \
    -i app.unsigned.apk \
    -k "$KEYS/key.pk8" \
    -c "$KEYS/cert.x509.pem" \
    -o app.apk || exit 1

if [ "${1}" = "--install" ]; then
    adb uninstall "$PACKAGE" 2>/dev/null || true
    adb install app.apk
    adb shell am start -n "$PACKAGE/.HelloWorld"
else
    echo "Built: app.apk"
    echo "To install:   sh build.sh --install"
    echo "To uninstall: sh build.sh --uninstall"
fi