#!/bin/sh
# Build the raycaster example.
# From the repo root: sh examples/raycaster/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=raycaster.asm
PACKAGE=app.hello.ray
ACTIVITY=.Main

. ../../tools/build.sh
