; ══════════════════════════════════════════════════════════
;  Native window — the frames are drawn in C, not here
;
;  class Main extends Activity
;             implements SurfaceHolder.Callback, Runnable {
;      SurfaceView view;
;
;      static native void start(Surface s);
;      static native void resize(int w, int h);
;      static native void frame();
;      static native void stop();
;
;      public void onCreate(Bundle b) {
;          view = new SurfaceView(this);
;          setContentView(view);
;          view.getHolder().addCallback(this);
;      }
;
;      public void surfaceCreated(SurfaceHolder h) {
;          start(h.getSurface());
;          view.postOnAnimation(this);
;      }
;      public void surfaceChanged(SurfaceHolder h, int f, int w, int ht) {
;          resize(w, ht);
;      }
;      public void surfaceDestroyed(SurfaceHolder h) { stop(); }
;
;      public void run() { frame(); view.postOnAnimation(this); }
;  }
;
;  Every other example that draws does it from bytecode — a Canvas,
;  a Paint, a Bitmap.  This one hands the Surface across the JNI
;  boundary once and never touches a pixel again: C locks the
;  window's own buffer, writes into it and posts it.  What is left
;  on this side is a Runnable that keeps asking for the next frame.
;
;  The C side reaches the window through libandroid.so, which comes
;  with the NDK — and the NDK is exactly what this repository does
;  without.  So stub/ holds a forged libandroid.so: the right
;  soname, the five names the linker is looking for, and nothing
;  behind them.  It satisfies the link; the device satisfies the
;  load.  build.sh checks afterwards that libandroid.so is the only
;  thing the library ended up asking for.
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _types
    deftype "Ljava/lang/Object;"
end macro

macro _class
    defclass "Lapp/hello/nativewin/Main;", ACC_PUBLIC, \
        "Landroid/app/Activity;", _win_ifaces

    deffield "->view:Landroid/view/SurfaceView;", ACC_PUBLIC

    ; Bound from JNI_OnLoad, so the C side keeps its own names.
    defmethod "->start(Landroid/view/Surface;)V", \
        ACC_PUBLIC or ACC_STATIC or ACC_NATIVE
    defmethod "->resize(II)V", \
        ACC_PUBLIC or ACC_STATIC or ACC_NATIVE
    defmethod "->frame()V", \
        ACC_PUBLIC or ACC_STATIC or ACC_NATIVE
    defmethod "->stop()V", \
        ACC_PUBLIC or ACC_STATIC or ACC_NATIVE

    method "-><init>()V", ACC_PUBLIC or ACC_CONSTRUCTOR, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 2
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1

        const_string   v0, "nativewin"
        invoke_static  "Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V", v0

        new_instance   v0, "Landroid/view/SurfaceView;"
        invoke_direct  "Landroid/view/SurfaceView;-><init>(Landroid/content/Context;)V", v0, p0
        iput_object    v0, p0, "->view:Landroid/view/SurfaceView;"
        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v0

        ;  The callback is what says when the Surface exists.  Ask
        ;  for it before one does, and there is nothing to draw on.
        invoke_virtual "Landroid/view/SurfaceView;->getHolder()Landroid/view/SurfaceHolder;", v0
        move_result_object v1
        invoke_interface "Landroid/view/SurfaceHolder;->addCallback(Landroid/view/SurfaceHolder$Callback;)V", v1, p0
        return_void
    end method

    method "->surfaceCreated(Landroid/view/SurfaceHolder;)V", ACC_PUBLIC, locals 2
        invoke_interface "Landroid/view/SurfaceHolder;->getSurface()Landroid/view/Surface;", p1
        move_result_object v0
        invoke_static  "Lapp/hello/nativewin/Main;->start(Landroid/view/Surface;)V", v0

        iget_object    v1, p0, "->view:Landroid/view/SurfaceView;"
        invoke_virtual "Landroid/view/View;->postOnAnimation(Ljava/lang/Runnable;)V", v1, p0
        return_void
    end method

    ;  The size arrives here and nowhere else, so this is where the
    ;  buffer geometry is set.
    method "->surfaceChanged(Landroid/view/SurfaceHolder;III)V", ACC_PUBLIC, locals 0
        invoke_static  "Lapp/hello/nativewin/Main;->resize(II)V", p3, p4
        return_void
    end method

    method "->surfaceDestroyed(Landroid/view/SurfaceHolder;)V", ACC_PUBLIC, locals 0
        invoke_static  "Lapp/hello/nativewin/Main;->stop()V"
        return_void
    end method

    ;  One frame, then ask for the next.  frame() returns at once if
    ;  the Surface has gone, so a post in flight when it does is
    ;  harmless.
    method "->run()V", ACC_PUBLIC, locals 1
        invoke_static  "Lapp/hello/nativewin/Main;->frame()V"
        iget_object    v0, p0, "->view:Landroid/view/SurfaceView;"
        invoke_virtual "Landroid/view/View;->postOnAnimation(Ljava/lang/Runnable;)V", v0, p0
        return_void
    end method
end macro

macro _interfaces
    _win_ifaces deftypelist "Landroid/view/SurfaceHolder$Callback;", \
                            "Ljava/lang/Runnable;"
end macro

dex_lists _class, _types

dex_file _interfaces
