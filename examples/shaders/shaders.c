/*
 * A fragment shader on the screen, with no NDK anywhere.
 *
 * EGL and GLES2 are platform libraries: on a normal Android build
 * they come from the NDK, headers and all.  There is no NDK here,
 * so this file declares the pieces of the ABI it uses — the
 * handful of types, the constants, the twenty-odd entry points —
 * and links against the stubs in stub/, which carry the sonames
 * and the names and nothing else.  The device supplies the code.
 *
 * What that buys is the shader below: once a GL context exists,
 * the interesting arithmetic happens on the GPU, written as text
 * and compiled by a driver that was on the phone all along.  The C
 * here is only plumbing, and the dex holds no drawing at all.
 */

#include "jni.h"

/* ── As much of EGL and GLES2 as this file touches ────────── */

typedef void          *EGLDisplay;
typedef void          *EGLSurface;
typedef void          *EGLContext;
typedef void          *EGLConfig;
typedef void          *EGLNativeWindowType;
typedef int32_t        EGLint;
typedef unsigned int   EGLBoolean;

typedef unsigned int   GLuint;
typedef int            GLint;
typedef unsigned int   GLenum;
typedef int            GLsizei;
typedef char           GLchar;
typedef float          GLfloat;
typedef unsigned char  GLboolean;

#define EGL_DEFAULT_DISPLAY       ((void *)0)
#define EGL_NO_CONTEXT            ((EGLContext)0)
#define EGL_NO_SURFACE            ((EGLSurface)0)
#define EGL_NO_DISPLAY            ((EGLDisplay)0)
#define EGL_RED_SIZE              0x3024
#define EGL_GREEN_SIZE            0x3023
#define EGL_BLUE_SIZE             0x3022
#define EGL_SURFACE_TYPE          0x3033
#define EGL_WINDOW_BIT            0x0004
#define EGL_RENDERABLE_TYPE       0x3040
#define EGL_OPENGL_ES2_BIT        0x0004
#define EGL_NATIVE_VISUAL_ID      0x302E
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_NONE                  0x3038

#define GL_FRAGMENT_SHADER        0x8B30
#define GL_VERTEX_SHADER          0x8B31
#define GL_COMPILE_STATUS         0x8B81
#define GL_LINK_STATUS            0x8B82
#define GL_FLOAT                  0x1406
#define GL_FALSE                  0
#define GL_TRIANGLE_STRIP         0x0005
#define GL_COLOR_BUFFER_BIT       0x00004000

extern void      *ANativeWindow_fromSurface(JNIEnv *env, jobject surface);
extern void       ANativeWindow_release(void *window);
extern int32_t    ANativeWindow_setBuffersGeometry(void *window, int32_t w,
                                                   int32_t h, int32_t format);

extern EGLDisplay eglGetDisplay(void *display_id);
extern EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor);
extern EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
                                  EGLConfig *configs, EGLint config_size,
                                  EGLint *num_config);
extern EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
                                     EGLint attribute, EGLint *value);
extern EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                                         EGLNativeWindowType win,
                                         const EGLint *attrib_list);
extern EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config,
                                   EGLContext share, const EGLint *attrib_list);
extern EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
                                 EGLSurface read, EGLContext ctx);
extern EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface);
extern EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface);
extern EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx);
extern EGLBoolean eglTerminate(EGLDisplay dpy);

extern GLuint glCreateShader(GLenum type);
extern void   glShaderSource(GLuint shader, GLsizei count,
                             const GLchar *const *string, const GLint *length);
extern void   glCompileShader(GLuint shader);
extern void   glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
extern void   glDeleteShader(GLuint shader);
extern GLuint glCreateProgram(void);
extern void   glAttachShader(GLuint program, GLuint shader);
extern void   glLinkProgram(GLuint program);
extern void   glGetProgramiv(GLuint program, GLenum pname, GLint *params);
extern void   glUseProgram(GLuint program);
extern GLint  glGetAttribLocation(GLuint program, const GLchar *name);
extern GLint  glGetUniformLocation(GLuint program, const GLchar *name);
extern void   glVertexAttribPointer(GLuint index, GLint size, GLenum type,
                                    GLboolean normalized, GLsizei stride,
                                    const void *pointer);
extern void   glEnableVertexAttribArray(GLuint index);
extern void   glUniform1f(GLint location, GLfloat v0);
extern void   glUniform2f(GLint location, GLfloat v0, GLfloat v1);
extern void   glDrawArrays(GLenum mode, GLint first, GLsizei count);
extern void   glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
extern void   glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
extern void   glClear(GLuint mask);

/* ── The shaders ──────────────────────────────────────────── */

static const char VERT[] =
    "attribute vec2 aPos;\n"
    "void main() { gl_Position = vec4(aPos, 0.0, 1.0); }\n";

/*
 * Ray marching against a signed distance field — the shape of
 * shader that every introduction starts with, and the one the
 * raycaster example is the flat ancestor of.
 *
 * map() answers, for any point in space, how far the nearest
 * surface is.  A ray then advances by exactly that much, which
 * cannot overshoot anything, and repeating it converges on the
 * first surface the ray meets.  No geometry is ever submitted:
 * the sphere and the floor exist only as that function, and the
 * normals come from sampling it four times around each hit.
 *
 * highp, not mediump: at mediump the distances stop resolving and
 * the surface breaks into bands.
 */
static const char FRAG[] =
    "precision highp float;\n"
    "uniform vec2  uRes;\n"
    "uniform float uTime;\n"
    "\n"
    "float map(vec3 p) {\n"
    "    float sphere = length(p - vec3(0.0, sin(uTime) * 0.35, 0.0)) - 1.0;\n"
    "    float floorY = p.y + 1.0;\n"
    "    return min(sphere, floorY);\n"
    "}\n"
    "\n"
    "vec3 normal(vec3 p) {\n"
    "    vec2 e = vec2(0.001, 0.0);\n"
    "    return normalize(vec3(map(p + e.xyy) - map(p - e.xyy),\n"
    "                          map(p + e.yxy) - map(p - e.yxy),\n"
    "                          map(p + e.yyx) - map(p - e.yyx)));\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec2 uv = (gl_FragCoord.xy * 2.0 - uRes) / min(uRes.x, uRes.y);\n"
    "\n"
    "    float a = uTime * 0.3;\n"
    "    vec3 ro = vec3(4.0 * sin(a), 1.4, 4.0 * cos(a));\n"
    "    vec3 f = normalize(vec3(0.0, 0.0, 0.0) - ro);\n"
    "    vec3 r = normalize(cross(vec3(0.0, 1.0, 0.0), f));\n"
    "    vec3 u = cross(f, r);\n"
    "    vec3 rd = normalize(uv.x * r + uv.y * u + 1.7 * f);\n"
    "\n"
    "    float t = 0.0;\n"
    "    for (int i = 0; i < 80; i++) {\n"
    "        float d = map(ro + rd * t);\n"
    "        if (d < 0.001 || t > 25.0) break;\n"
    "        t += d;\n"
    "    }\n"
    "\n"
    "    vec3 col = vec3(0.05, 0.07, 0.10);\n"
    "    if (t < 25.0) {\n"
    "        vec3 p = ro + rd * t;\n"
    "        vec3 n = normal(p);\n"
    "        vec3 l = normalize(vec3(0.7, 0.9, 0.4));\n"
    "        float dif = max(dot(n, l), 0.0);\n"
    "        float spe = pow(max(dot(reflect(-l, n), -rd), 0.0), 32.0);\n"
    "\n"
    "        vec3 base = vec3(0.85, 0.45, 0.25);\n"
    "        if (p.y < -0.99) {\n"
    "            vec2 q = floor(p.xz);\n"
    "            base = mix(vec3(0.20), vec3(0.32), mod(q.x + q.y, 2.0));\n"
    "        }\n"
    "        col = base * (0.15 + 0.85 * dif) + spe * 0.4;\n"
    "        col = mix(col, vec3(0.05, 0.07, 0.10), 1.0 - exp(-0.02 * t * t));\n"
    "    }\n"
    "\n"
    "    gl_FragColor = vec4(sqrt(col), 1.0);\n"
    "}\n";

static const GLfloat QUAD[] = {
    -1.0f, -1.0f,
     1.0f, -1.0f,
    -1.0f,  1.0f,
     1.0f,  1.0f,
};

/* ── State ────────────────────────────────────────────────── */

static void      *win;
static EGLDisplay dpy = EGL_NO_DISPLAY;
static EGLSurface surf = EGL_NO_SURFACE;
static EGLContext ctx = EGL_NO_CONTEXT;
static GLuint     prog;
static GLint      loc_res, loc_time, loc_pos;
static GLfloat    width, height;
static int32_t    tick;

static GLuint compile(GLenum type, const char *src)
{
    GLuint sh = glCreateShader(type);
    GLint ok = GL_FALSE;

    glShaderSource(sh, 1, &src, 0);
    glCompileShader(sh);
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static void start(JNIEnv *env, jclass cls, jobject surface)
{
    const EGLint config_attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_NONE
    };
    const EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLConfig config;
    EGLint n = 0, format = 0;
    GLuint vs, fs;

    (void)cls;

    win = ANativeWindow_fromSurface(env, surface);
    if (!win)
        return;

    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(dpy, 0, 0);
    eglChooseConfig(dpy, config_attrs, &config, 1, &n);
    if (n < 1)
        return;

    /*  The window has to be told the pixel format the chosen
     *  config wants, or the surface comes out wrong on some
     *  drivers.  EGL knows it as the native visual id. */
    eglGetConfigAttrib(dpy, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(win, 0, 0, format);

    surf = eglCreateWindowSurface(dpy, config, win, 0);
    ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, ctx_attrs);
    eglMakeCurrent(dpy, surf, surf, ctx);

    vs = compile(GL_VERTEX_SHADER, VERT);
    fs = compile(GL_FRAGMENT_SHADER, FRAG);
    if (!vs || !fs)
        return;

    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glUseProgram(prog);

    loc_pos  = glGetAttribLocation(prog, "aPos");
    loc_res  = glGetUniformLocation(prog, "uRes");
    loc_time = glGetUniformLocation(prog, "uTime");

    glVertexAttribPointer((GLuint)loc_pos, 2, GL_FLOAT, GL_FALSE, 0, QUAD);
    glEnableVertexAttribArray((GLuint)loc_pos);
}

static void resize(JNIEnv *env, jclass cls, jint w, jint h)
{
    (void)env; (void)cls;
    width = (GLfloat)w;
    height = (GLfloat)h;
    glViewport(0, 0, w, h);
}

/*
 * Time comes from counting frames rather than from a clock: there
 * is no libc here to ask, and the surface is posted on vsync, so
 * a sixtieth per frame is close enough for something with no idea
 * what time it is.
 */
static void frame(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;

    if (ctx == EGL_NO_CONTEXT)
        return;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUniform2f(loc_res, width, height);
    glUniform1f(loc_time, (GLfloat)(tick++) / 60.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    eglSwapBuffers(dpy, surf);
}

static void stop(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;

    if (dpy != EGL_NO_DISPLAY) {
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (ctx != EGL_NO_CONTEXT)
            eglDestroyContext(dpy, ctx);
        if (surf != EGL_NO_SURFACE)
            eglDestroySurface(dpy, surf);
        eglTerminate(dpy);
    }
    if (win)
        ANativeWindow_release(win);

    dpy = EGL_NO_DISPLAY;
    surf = EGL_NO_SURFACE;
    ctx = EGL_NO_CONTEXT;
    win = 0;
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

    cls = (*env)->FindClass(env, "app/hello/shaders/Main");
    if (!cls)
        return JNI_ERR;

    if ((*env)->RegisterNatives(env, cls, METHODS,
                                sizeof METHODS / sizeof METHODS[0]) != JNI_OK)
        return JNI_ERR;

    return JNI_VERSION_1_6;
}
