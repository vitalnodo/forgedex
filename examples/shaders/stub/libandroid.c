/* A stand-in for Android's libandroid.so.  See stub/libEGL.c. */

#define STUB(name) void name(void) { }

STUB(ANativeWindow_fromSurface)
STUB(ANativeWindow_release)
STUB(ANativeWindow_setBuffersGeometry)
