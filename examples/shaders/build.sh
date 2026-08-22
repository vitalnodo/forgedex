#!/bin/sh
# Build the shaders example.
# From the repo root: sh examples/shaders/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=shaders.asm
PACKAGE=app.hello.shaders
ACTIVITY=.Main

# EGL and GLES2 come with the NDK, which is not here.  stub/ carries
# their sonames and their symbol names; the device carries the code.
NATIVE_STUBS="stub/libandroid.c stub/libEGL.c stub/libGLESv2.c"
NATIVE_SRC="shaders.c stub/libandroid.so stub/libEGL.so stub/libGLESv2.so"
NATIVE_LIB=libshaders.so
NATIVE_CFLAGS="-O2"
NATIVE_NEEDS="libandroid.so libEGL.so libGLESv2.so"


. ../../tools/build.sh
