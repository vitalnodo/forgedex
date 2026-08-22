; ══════════════════════════════════════════════════════════
;  Themes — one layout, thirteen windows, swipe between them
;
;  class T3 extends Activity {                  // …and T0…T12
;      int downX;
;      public void onCreate(Bundle b) {
;          super.onCreate(b);
;          setContentView(R.layout.screen);     // the same for all
;          setTitle("Theme.Material.Dialog");
;          ((TextView) findViewById(R.id.label))
;              .setText("Theme.Material.Dialog");
;      }
;      public boolean dispatchTouchEvent(MotionEvent e) {
;          ...a fling left starts T4, a fling right starts T2...
;          return super.dispatchTouchEvent(e);
;      }
;  }
;
;  A theme is picked when a window is created and never after, so
;  showing thirteen of them means thirteen activities.  Every one
;  of them is the same two methods over a different name and a
;  different neighbour, and none of them is written out: the macro
;  below is expanded once per entry in list.inc, and the whole dex
;  — fourteen classes, twenty-eight methods — comes out of one
;  `each_theme _screen`.
;
;  The swipe is read in dispatchTouchEvent rather than through an
;  OnTouchListener, because the listener on the root view never
;  hears a touch that a button or a slider took first, and this
;  screen is nothing but buttons and sliders.  dispatchTouchEvent
;  sees every touch before any child does; anything that is not a
;  fling is handed straight on to super, so the widgets go on
;  working.
;
;  There is no cmp-float in the DSL yet, so the coordinates are
;  cut to int as they arrive and the whole gesture is integer
;  arithmetic.
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'
include 'ids.inc'
include 'list.inc'

;  Ours, from res.asm: package $7F, type, entry.

;  The framework's, package $01 type $0A — the only transitions
;  that are public.  There is no slide_in_right to pair with
;  slide_out_right, so going back fades instead of sliding.
ANIM_SLIDE_IN?   := $010A0002
ANIM_SLIDE_OUT?  := $010A0003
ANIM_FADE_IN?    := $010A0000
ANIM_FADE_OUT?   := $010A0001

;  How far a finger has to travel before it counts as a fling.
SWIPE? := 160

macro _types
    deftype "Ljava/lang/Object;"
end macro

; ── One themed window ─────────────────────────────────────

macro _screen? label?*, style?*
    theme_class __ti
    __this = __cls
    theme_class ((__ti + $01) mod THEME_COUNT)
    __next = __cls
    theme_class ((__ti + THEME_COUNT - $01) mod THEME_COUNT)
    __prev = __cls

    defclass __this, ACC_PUBLIC, "Landroid/app/Activity;"

    deffield "->downX:I", ACC_PRIVATE

    method "-><init>()V", ACC_PUBLIC or ACC_CONSTRUCTOR, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    ; v0=resource id  v1=the name  v2=the label view
    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 3
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1

        const          v0, R_LAYOUT_SCREEN
        invoke_virtual "Landroid/app/Activity;->setContentView(I)V", p0, v0

        ;  The title bar names the theme, but half of these themes
        ;  have no title bar — so the layout carries a TextView for
        ;  the name as well, and it is found by the id res.asm
        ;  minted for it.
        const_string   v1, label
        invoke_virtual "Landroid/app/Activity;->setTitle(Ljava/lang/CharSequence;)V", p0, v1

        const          v0, R_ID_LABEL
        invoke_virtual "Landroid/app/Activity;->findViewById(I)Landroid/view/View;", p0, v0
        move_result_object v2
        check_cast     v2, "Landroid/widget/TextView;"
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v2, v1
        return_void
    end method

    ; v0=action, then the class to go to  v1=x, then dx, then the
    ; intent  v2,v3=scratch, then the pair of animations
    method "->dispatchTouchEvent(Landroid/view/MotionEvent;)Z", ACC_PUBLIC, locals 4
        invoke_virtual "Landroid/view/MotionEvent;->getAction()I", p1
        move_result    v0
        if_eqz         v0, .down                  ; ACTION_DOWN
        const_4        v2, 1
        if_eq          v0, v2, .up                ; ACTION_UP
        goto           .pass

    .down:
        invoke_virtual "Landroid/view/MotionEvent;->getX()F", p1
        move_result    v1
        float_to_int   v1, v1
        iput           v1, p0, "->downX:I"
        goto           .pass

    .up:
        invoke_virtual "Landroid/view/MotionEvent;->getX()F", p1
        move_result    v1
        float_to_int   v1, v1
        iget           v2, p0, "->downX:I"
        sub_int        v1, v1, v2                 ; how far, and which way
        const          v2, SWIPE
        if_gt          v1, v2, .backward
        const          v2, -SWIPE
        if_lt          v1, v2, .forward
        goto           .pass

    .forward:
        const_class    v0, __next
        const          v2, ANIM_SLIDE_IN
        const          v3, ANIM_SLIDE_OUT
        goto           .go
    .backward:
        const_class    v0, __prev
        const          v2, ANIM_FADE_IN
        const          v3, ANIM_FADE_OUT
    .go:
        new_instance   v1, "Landroid/content/Intent;"
        invoke_direct  "Landroid/content/Intent;-><init>(Landroid/content/Context;Ljava/lang/Class;)V", v1, p0, v0
        invoke_virtual "Landroid/app/Activity;->startActivity(Landroid/content/Intent;)V", p0, v1
        ;  The animation has to be asked for after the start it
        ;  belongs to, not before: it applies to the transition
        ;  already under way.
        invoke_virtual "Landroid/app/Activity;->overridePendingTransition(II)V", p0, v2, v3
        ;  The window we are leaving is finished, so the stack stays
        ;  one deep however long the gallery is walked and back
        ;  leaves the app rather than unwinding thirteen windows.
        invoke_virtual "Landroid/app/Activity;->finish()V", p0
        const_4        v0, 1                      ; the touch was ours
        return         v0

    .pass:
        invoke_super   "Landroid/app/Activity;->dispatchTouchEvent(Landroid/view/MotionEvent;)Z", p0, p1
        move_result    v0
        return         v0
    end method

    __ti = __ti + $01
end macro

macro _screens
    __ti = $00
    each_theme _screen
end macro

dex_lists _screens, _types

dex_file
