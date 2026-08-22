; ══════════════════════════════════════════════════════════
;  JNI Hello
;
;  public class HelloWorld extends Activity {
;      public HelloWorld() { super(); }
;      public native String answer();
;
;      public void onCreate(Bundle b) {
;          super.onCreate(b);
;          System.loadLibrary("hello");
;          TextView tv = new TextView(this);
;          tv.setText(answer());
;          setContentView(tv);
;      }
;  }
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _protos
    ; nothing calls a method with this signature, but the pool
    ; is meant to carry it
    defproto _I, "I", <>
end macro

macro _methods
    ; nothing calls this, but the pool is meant to carry it
    defmethod "Ljava/lang/Integer;->toString(I)Ljava/lang/String;"
end macro

macro _types
    deftype "Ljava/lang/Object;"
end macro

macro _class
    defclass "Lapp/hello/jni/HelloWorld;", ACC_PUBLIC, \
        "Landroid/app/Activity;"
    defmethod "->answer()Ljava/lang/String;", ACC_PUBLIC or ACC_NATIVE

    method "-><init>()V", ACC_PUBLIC or ACC_CONSTRUCTOR, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0   ; Activity.<init>
        return_void
    end method

    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 2
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1
        const_string   v0, "hello"
        invoke_static  "Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V", v0
        new_instance   v1, "Landroid/widget/TextView;"                   ; new TextView
        invoke_direct  "Landroid/widget/TextView;-><init>(Landroid/content/Context;)V", v1, p0                   ; TextView.<init>
        invoke_virtual "->answer()Ljava/lang/String;", p0   ; this.answer()
        move_result_object v0
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v1, v0
        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v1
        return_void
    end method
end macro

dex_lists _class, _types, _protos, _methods

dex_file
