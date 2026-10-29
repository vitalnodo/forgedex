#!/bin/sh
# Build the selftest example.
# From the repo root: sh examples/selftest/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=selftest.asm
PACKAGE=app.hello.selftest
ACTIVITY=.Main

. ../../tools/build.sh
