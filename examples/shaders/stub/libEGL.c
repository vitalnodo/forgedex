/*
 * A stand-in for Android's libEGL.so, for link time only.
 *
 * Only the names matter: a shared library's dynamic symbols carry
 * no signature the linker could check against, so a stub needs no
 * argument list and no body.  The soname is what makes the loader
 * ask the device for the real one instead.
 */

#define STUB(name) void name(void) { }

STUB(eglGetDisplay)
STUB(eglInitialize)
STUB(eglChooseConfig)
STUB(eglGetConfigAttrib)
STUB(eglCreateWindowSurface)
STUB(eglCreateContext)
STUB(eglMakeCurrent)
STUB(eglSwapBuffers)
STUB(eglDestroySurface)
STUB(eglDestroyContext)
STUB(eglTerminate)
