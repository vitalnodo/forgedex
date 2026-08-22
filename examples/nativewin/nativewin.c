/*
 * Pixels straight into the window, with no Java in the way.
 *
 * A SurfaceView hands its Surface down here once, and from then on
 * every frame is locked, written and posted by this file.  Not one
 * drawing instruction survives in the dex: the Dalvik side only
 * posts a Runnable to keep the frames coming.
 *
 * All of it goes through libandroid.so, which the NDK would
 * normally supply.  We link against stub/libandroid.so instead —
 * the right soname, the right five names, and nothing behind them.
 * The device supplies the real one at load time.  build.sh checks
 * that libandroid.so is the only thing we ended up asking for.
 *
 * The ABI below is declared here for the same reason: there is no
 * android/native_window.h to include, so the struct is written out
 * to match the one the platform documents.
 */

#include "jni.h"

typedef struct ANativeWindow ANativeWindow;

typedef struct ARect {
    int32_t left, top, right, bottom;
} ARect;

typedef struct ANativeWindow_Buffer {
    int32_t  width;
    int32_t  height;
    int32_t  stride;       /* in pixels, not bytes, and rarely == width */
    int32_t  format;
    void    *bits;
    uint32_t reserved[6];
} ANativeWindow_Buffer;

#define WINDOW_FORMAT_RGBX_8888 2

extern ANativeWindow *ANativeWindow_fromSurface(JNIEnv *env, jobject surface);
extern void  ANativeWindow_release(ANativeWindow *window);
extern int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *window,
                                                int32_t w, int32_t h,
                                                int32_t format);
extern int32_t ANativeWindow_lock(ANativeWindow *window,
                                  ANativeWindow_Buffer *out, ARect *dirty);
extern int32_t ANativeWindow_unlockAndPost(ANativeWindow *window);

/* sin, scaled to 0..255 and sampled 256 times round the circle.
 * There is no libm to ask, and this is all the plasma wants. */
static const unsigned char SIN[256] = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
    176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
    218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
    245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255,
    255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246,
    245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220,
    218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179,
    176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131,
    128, 124, 121, 118, 115, 112, 109, 106, 103, 100,  97,  93,  90,  88,  85,  82,
     79,  76,  73,  70,  67,  65,  62,  59,  57,  54,  52,  49,  47,  44,  42,  40,
     37,  35,  33,  31,  29,  27,  25,  23,  21,  20,  18,  17,  15,  14,  12,  11,
     10,   9,   7,   6,   5,   5,   4,   3,   2,   2,   1,   1,   1,   0,   0,   0,
      0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   4,   5,   5,   6,   7,   9,
     10,  11,  12,  14,  15,  17,  18,  20,  21,  23,  25,  27,  29,  31,  33,  35,
     37,  40,  42,  44,  47,  49,  52,  54,  57,  59,  62,  65,  67,  70,  73,  76,
     79,  82,  85,  88,  90,  93,  97, 100, 103, 106, 109, 112, 115, 118, 121, 124,
};

static ANativeWindow *win;
static int32_t tick;

/*
 * The buffer is asked for at a third of the window's size and the
 * compositor scales it up.  A phone screen is some two and a half
 * million pixels, and this loop is plain scalar C.
 */
static void resize(JNIEnv *env, jclass cls, jint w, jint h)
{
    (void)env; (void)cls;
    if (win)
        ANativeWindow_setBuffersGeometry(win, w / 3, h / 3,
                                         WINDOW_FORMAT_RGBX_8888);
}

static void start(JNIEnv *env, jclass cls, jobject surface)
{
    (void)cls;
    if (win)
        ANativeWindow_release(win);
    win = ANativeWindow_fromSurface(env, surface);
}

static void stop(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    if (win) {
        ANativeWindow_release(win);
        win = 0;
    }
}

/*
 * Three travelling waves summed and read back through the table as
 * a colour.  Everything is a byte index, so nothing here needs a
 * multiply wider than the one the compiler already had.
 */
static void frame(JNIEnv *env, jclass cls)
{
    ANativeWindow_Buffer buf;
    uint32_t *row;
    int32_t x, y, t;
    unsigned v;

    (void)env; (void)cls;

    if (!win || ANativeWindow_lock(win, &buf, 0) != 0)
        return;

    t = tick++;

    for (y = 0; y < buf.height; y++) {
        row = (uint32_t *)buf.bits + (long)y * buf.stride;
        for (x = 0; x < buf.width; x++) {
            v = (unsigned)(SIN[(x * 2 + t) & 255]
                         + SIN[(y * 3 - t) & 255]
                         + SIN[(x + y + t * 2) & 255]) / 3u;

            /* RGBX_8888 is R in the low byte, so the shifts run
             * the other way round from the name. */
            row[x] = 0xFF000000u
                   | ((uint32_t)SIN[(v + 170) & 255] << 16)
                   | ((uint32_t)SIN[(v + 85) & 255] << 8)
                   |  (uint32_t)SIN[v & 255];
        }
    }

    ANativeWindow_unlockAndPost(win);
}

static const JNINativeMethod METHODS[] = {
    { "start",  "(Landroid/view/Surface;)V", (void *)start  },
    { "resize", "(II)V",                     (void *)resize },
    { "frame",  "()V",                       (void *)frame  },
    { "stop",   "()V",                       (void *)stop   },
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env;
    jclass cls;

    (void)reserved;

    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;

    cls = (*env)->FindClass(env, "app/hello/nativewin/Main");
    if (!cls)
        return JNI_ERR;

    if ((*env)->RegisterNatives(env, cls, METHODS,
                                sizeof METHODS / sizeof METHODS[0]) != JNI_OK)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}
