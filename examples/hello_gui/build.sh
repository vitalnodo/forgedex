#!/bin/sh
# Build the hello_gui example.
# From the repo root: sh examples/hello_gui/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=hello.asm
PACKAGE=app.hello
ACTIVITY=.HelloWorld

. ../../tools/build.sh
