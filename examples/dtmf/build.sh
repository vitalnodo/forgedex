#!/bin/sh
# Build the dtmf example.
# From the repo root: sh examples/dtmf/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=dtmf.asm
PACKAGE=app.hello.dtmf
ACTIVITY=.Dialer

# prog.c is the IOCCC entry, compiled unmodified: -I shim puts our
# own <stdio.h> and <string.h> in front of the system ones, and
# -Dmain renames its main so that the JNI side can call it.
NATIVE_SRC="dtmf.c shim/shim.c ioccc/prog.c"
NATIVE_LIB=libdtmf.so
NATIVE_CFLAGS="-I shim -Dmain=dtmf_main"
ASSET_FILES=ioccc/dtmf.bin

. ../../tools/build.sh
