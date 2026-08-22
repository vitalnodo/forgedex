#include <string.h>
#include "jni.h"

JNIEXPORT jstring JNICALL
Java_app_hello_jni_HelloWorld_answer(JNIEnv* env, jobject thiz)
{
    return (*env)->NewStringUTF(env, "Hello from hello-jni.c!");
}
