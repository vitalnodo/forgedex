; ══════════════════════════════════════════════════════════
;  FizzBuzz — tap the button, the label shows the next term
;
;  public class FizzBuzz extends Activity
;                        implements View.OnClickListener {
;      int count = 0;
;      TextView tv;
;
;      public void onCreate(Bundle b) { ...LinearLayout with tv + button... }
;
;      public void onClick(View v) {
;          count++;
;          if      (count % 15 == 0) tv.setText("FizzBuzz");
;          else if (count %  3 == 0) tv.setText("Fizz");
;          else if (count %  5 == 0) tv.setText("Buzz");
;          else                      tv.setText(Integer.toString(count));
;      }
;  }
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _types
    deftype "Ljava/lang/Object;"
end macro

macro _class
    defclass "Lapp/hello/fizzbuzz/FizzBuzz;", ACC_PUBLIC, \
        "Landroid/app/Activity;", _fizz_ifaces

    deffield "->count:I", ACC_PUBLIC
    deffield "->tv:Landroid/widget/TextView;", ACC_PUBLIC

    method "-><init>()V", ACC_PUBLIC, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    ; v0=layout  v1=label  v2=button  v3=scratch
    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 4
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1

        new_instance   v0, "Landroid/widget/LinearLayout;"
        invoke_direct  "Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V", v0, p0
        const_4        v3, 1                                    ; LinearLayout.VERTICAL
        invoke_virtual "Landroid/widget/LinearLayout;->setOrientation(I)V", v0, v3

        new_instance   v1, "Landroid/widget/TextView;"
        invoke_direct  "Landroid/widget/TextView;-><init>(Landroid/content/Context;)V", v1, p0
        const_string   v3, "tap to start"
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v1, v3
        iput_object    v1, p0, "->tv:Landroid/widget/TextView;"

        new_instance   v2, "Landroid/widget/Button;"
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v2, p0
        const_string   v3, "next"
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v2, v3
        invoke_virtual "Landroid/widget/Button;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v2, p0

        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v0, v1
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v0, v2
        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v0
        return_void
    end method

    ; v0=count  v1=remainder  v2=label  v3=text to show
    method "->onClick(Landroid/view/View;)V", ACC_PUBLIC, locals 4
        iget           v0, p0, "->count:I"
        add_int_lit8   v0, v0, 1
        iput           v0, p0, "->count:I"
        iget_object    v2, p0, "->tv:Landroid/widget/TextView;"

        rem_int_lit8   v1, v0, 15
        if_nez         v1, .not_both
        const_string   v3, "FizzBuzz"
        goto           .show
    .not_both:
        rem_int_lit8   v1, v0, 3
        if_nez         v1, .not_three
        const_string   v3, "Fizz"
        goto           .show
    .not_three:
        rem_int_lit8   v1, v0, 5
        if_nez         v1, .plain
        const_string   v3, "Buzz"
        goto           .show
    .plain:
        invoke_static  "Ljava/lang/Integer;->toString(I)Ljava/lang/String;", v0
        move_result_object v3
    .show:
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v2, v3
        return_void
    end method
end macro

macro _interfaces
    _fizz_ifaces deftypelist "Landroid/view/View$OnClickListener;"
end macro

dex_lists _class, _types

dex_file _interfaces
