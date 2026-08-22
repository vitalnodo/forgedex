format binary as "dex"
org $00

include '../../forgedex.inc'

macro _types
    deftype "Ljava/lang/Object;"
end macro

macro _class
    defclass "Lapp/hello/HelloWorld;", ACC_PUBLIC, \
        "Landroid/app/Activity;"
    method "-><init>()V", ACC_PUBLIC, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 2
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1
        new_instance   v0, "Landroid/widget/TextView;"
        invoke_direct  "Landroid/widget/TextView;-><init>(Landroid/content/Context;)V", v0, p0
        const_string   v1, "Hello, World!"
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v0, v1
        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v0
        return_void
    end method
end macro

dex_lists _class, _types

dex_file
