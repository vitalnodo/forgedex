#!/bin/sh
# Build the fetch example.
# From the repo root: sh examples/fetch/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=fetch.asm
PACKAGE=app.hello.fetch
ACTIVITY=.Main

. ../../tools/build.sh
