/*
 * The seam between Dalvik and the IOCCC entry.
 *
 * Dalvik hands down two things: the bytes of dtmf.bin, which it
 * read out of the APK's assets, and the digit that was pressed.
 * We point the shim's fopen at those bytes, call the entry's main
 * the way a shell would, and hand back the samples it wrote.
 *
 * The entry writes a whole WAV — a 44-byte header and then 16-bit
 * samples at 16 kHz.  AudioTrack wants the samples alone, so the
 * header is stepped over here rather than in bytecode.
 *
 * Nothing in this file, in shim.c, or in prog.c calls libc, so the
 * library that comes out has no DT_NEEDED at all and loads under
 * bionic without ever having heard of it.  build.sh checks that.
 */

#include "jni.h"
#include "stdio.h"
#include "string.h"

#define WAV_HEADER 44
#define DIGITS_MAX 64

static jshortArray tone(JNIEnv *env, jclass cls, jbyteArray bin, jstring digits)
{
    static char pressed[DIGITS_MAX];
    char *argv[3];
    jbyte *bytes;
    const char *utf;
    unsigned char *wav;
    long len;
    jsize samples;
    jshortArray out;
    size_t n;

    (void)cls;

    /* dtmf.bin, as read from assets/ on the Dalvik side */
    bytes = (*env)->GetByteArrayElements(env, bin, 0);
    if (!bytes)
        return 0;
    dtmf_set_file((const unsigned char *)bytes,
                  (long)(*env)->GetArrayLength(env, bin));

    /* argv, as a shell would have built it */
    utf = (*env)->GetStringUTFChars(env, digits, 0);
    if (!utf) {
        (*env)->ReleaseByteArrayElements(env, bin, bytes, JNI_ABORT);
        return 0;
    }
    n = strlen(utf);
    if (n > DIGITS_MAX - 1)
        n = DIGITS_MAX - 1;
    memcpy(pressed, utf, n);
    pressed[n] = 0;
    (*env)->ReleaseStringUTFChars(env, digits, utf);

    argv[0] = "prog";
    argv[1] = pressed;
    argv[2] = 0;

    dtmf_rewind_output();
    dtmf_main(2, argv);
    len = dtmf_output(&wav);

    (*env)->ReleaseByteArrayElements(env, bin, bytes, JNI_ABORT);

    if (len <= WAV_HEADER)
        return 0;

    samples = (jsize)((len - WAV_HEADER) / 2);
    out = (*env)->NewShortArray(env, samples);
    if (!out)
        return 0;
    (*env)->SetShortArrayRegion(env, out, 0, samples,
                                (const jshort *)(wav + WAV_HEADER));
    return out;
}

/*
 * Bound from JNI_OnLoad rather than found by name, so the symbol
 * above is plain `tone` and not Java_app_hello_dtmf_Dialer_tone.
 */
static const JNINativeMethod METHODS[] = {
    { "tone", "([BLjava/lang/String;)[S", (void *)tone },
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env;
    jclass cls;

    (void)reserved;

    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;

    cls = (*env)->FindClass(env, "app/hello/dtmf/Dialer");
    if (!cls)
        return JNI_ERR;

    if ((*env)->RegisterNatives(env, cls, METHODS,
                                sizeof METHODS / sizeof METHODS[0]) != JNI_OK)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}
