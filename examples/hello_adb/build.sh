#!/bin/sh
# Run from repo root: sh examples/hello_adb/build.sh
# Or from this directory: sh build.sh
 
cd "$(dirname "$0")"
 
FASMG="../../deps/fasmg"
 
if [ ! -f "$FASMG" ]; then
    echo "Error: deps not found. Run first: sh ../../setup_deps.sh"
    exit 1
fi
 
"$FASMG" hello.inc classes.dex || exit 1
 
zip HelloWorld.apk classes.dex
 
adb push HelloWorld.apk /data/local/tmp
adb shell dalvikvm -cp /data/local/tmp/HelloWorld.apk HelloWorld
