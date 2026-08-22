#!/bin/sh
# Build the themes example.
# From the repo root: sh examples/themes/build.sh
# From here:          sh build.sh

cd "$(dirname "$0")"

ASM=themes.asm
PACKAGE=app.hello.themes
ACTIVITY=.T0

#  The only example so far with a resource table: an icon, a layout
#  and an id, all declared once in ids.inc.
RES_ASM=res.asm
LAYOUT_ASM=screen.asm
RES_FILES="res/drawable/icon.png"

. ../../tools/build.sh
