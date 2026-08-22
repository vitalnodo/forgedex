#!/bin/sh
# Build the hello_jni example.
# From the repo root: sh examples/hello_jni/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=jni.asm
PACKAGE=app.hello.jni
ACTIVITY=.HelloWorld
NATIVE_SRC=hello-jni.c
NATIVE_LIB=libhello.so

. ../../tools/build.sh
