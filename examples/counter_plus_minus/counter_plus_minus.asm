; ══════════════════════════════════════════════════════════
;  Counter2 — TextView + Button(+) + Button(-) in LinearLayout
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _protos
    defproto _oncl,    "V",                  <"Landroid/view/View$OnClickListener;">
end macro

macro _types
    deftype "Ljava/lang/Object;"
end macro

macro _class
    defclass "Lapp/hello/counter_plus_minus/HelloWorld;", ACC_PUBLIC, \
        "Landroid/app/Activity;", _hello_ifaces
    deffield "->btn_minus:Landroid/widget/Button;", ACC_PUBLIC
    deffield "->btn_plus:Landroid/widget/Button;", ACC_PUBLIC
    deffield "->count:I", ACC_PUBLIC
    deffield "->tv:Landroid/widget/TextView;", ACC_PUBLIC
    method "-><init>()V", ACC_PUBLIC, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0   ; Activity.<init>
        return_void
    end method

    method "->onClick(Landroid/view/View;)V", ACC_PUBLIC, locals 4
        iget_object    v1, p0, "->btn_plus:Landroid/widget/Button;"
        if_eq          p1, v1, .plus   ; if view == btn_plus
        iget           v0, p0, "->count:I"
        if_eqz         v0, .upd   ; if count == 0
        add_int_lit8   v0, v0, -1   ; count--
        goto           .upd
    .plus:
        iget           v0, p0, "->count:I"
        add_int_lit8   v0, v0, 1   ; count++
    .upd:
        iput           v0, p0, "->count:I"
        iget_object    v2, p0, "->tv:Landroid/widget/TextView;"
        invoke_static  "Ljava/lang/Integer;->toString(I)Ljava/lang/String;", v0
        move_result_object v0
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v2, v0
        return_void
    end method

    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 5
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1
        new_instance   v0, "Landroid/widget/LinearLayout;"   ; new LinearLayout
        invoke_direct  "Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V", v0, p0                   ; LinearLayout.<init>
        const_4        v4, 1   ; VERTICAL
        invoke_virtual "Landroid/widget/LinearLayout;->setOrientation(I)V", v0, v4
        new_instance   v1, "Landroid/widget/TextView;"                   ; new TextView
        invoke_direct  "Landroid/widget/TextView;-><init>(Landroid/content/Context;)V", v1, p0                   ; TextView.<init>
        const_string   v4, "0"
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v1, v4
        iput_object    v1, p0, "->tv:Landroid/widget/TextView;"   ; this.tv = tv
        new_instance   v2, "Landroid/widget/Button;"   ; new Button
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v2, p0                  ; Button.<init>
        const_string   v4, "+"
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v2, v4
        invoke_virtual "Landroid/widget/Button;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v2, p0
        iput_object    v2, p0, "->btn_plus:Landroid/widget/Button;"                  ; this.btn_plus = btn
        new_instance   v3, "Landroid/widget/Button;"   ; new Button
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v3, p0                  ; Button.<init>
        const_string   v4, "-"
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v3, v4
        invoke_virtual "Landroid/widget/Button;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v3, p0
        iput_object    v3, p0, "->btn_minus:Landroid/widget/Button;"                 ; this.btn_minus = btn
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v0, v1                ; addView(tv)
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v0, v3                ; addView(btn_minus)
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v0, v2                ; addView(btn_plus)
        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v0
        return_void
    end method
end macro

dex_lists _class, _types, _protos

macro _interfaces
    _hello_ifaces = _oncl_tl
end macro

dex_file _interfaces

