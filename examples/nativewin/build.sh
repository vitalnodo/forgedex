#!/bin/sh
# Build the nativewin example.
# From the repo root: sh examples/nativewin/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=nativewin.asm
PACKAGE=app.hello.nativewin
ACTIVITY=.Main

# stub/libandroid.so carries the names the linker wants; the device
# carries the code.  NATIVE_NEEDS says what may show up as a
# DT_NEEDED, and build.sh fails on anything else.
NATIVE_STUBS="stub/libandroid.c"
NATIVE_SRC="nativewin.c stub/libandroid.so"
NATIVE_LIB=libnativewin.so
NATIVE_CFLAGS="-O2"
NATIVE_NEEDS="libandroid.so"

. ../../tools/build.sh
