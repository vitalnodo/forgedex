#!/bin/sh
# Build the fizzbuzz example.
# From the repo root: sh examples/fizzbuzz/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=fizzbuzz.asm
PACKAGE=app.hello.fizzbuzz
ACTIVITY=.FizzBuzz

. ../../tools/build.sh
