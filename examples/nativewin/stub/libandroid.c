/*
 * A stand-in for Android's libandroid.so, for link time only.
 *
 * The NDK is not here, so nothing supplies these symbols when the
 * library is linked.  This file supplies the names, with nothing
 * behind them, under the soname the loader will look for; on the
 * device the real libandroid.so answers instead, and none of the
 * code below is ever reached.
 *
 * Keep this in step with the declarations in nativewin.c.  Only
 * the names and the soname matter here — not the signatures, and
 * certainly not the bodies.
 */

void *ANativeWindow_fromSurface(void *env, void *surface)
{
    (void)env; (void)surface;
    return 0;
}

void ANativeWindow_release(void *window)
{
    (void)window;
}

int ANativeWindow_setBuffersGeometry(void *window, int w, int h, int format)
{
    (void)window; (void)w; (void)h; (void)format;
    return 0;
}

int ANativeWindow_lock(void *window, void *out, void *dirty)
{
    (void)window; (void)out; (void)dirty;
    return 0;
}

int ANativeWindow_unlockAndPost(void *window)
{
    (void)window;
    return 0;
}
