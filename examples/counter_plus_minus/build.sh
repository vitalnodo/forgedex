#!/bin/sh
# Build the counter_plus_minus example.
# From the repo root: sh examples/counter_plus_minus/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=counter_plus_minus.asm
PACKAGE=app.hello.counter_plus_minus
ACTIVITY=.HelloWorld

. ../../tools/build.sh
