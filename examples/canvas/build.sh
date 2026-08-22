#!/bin/sh
# Build the canvas example.
# From the repo root: sh examples/canvas/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=canvas.asm
PACKAGE=app.hello.canvas
ACTIVITY=.CanvasDemo

. ../../tools/build.sh
