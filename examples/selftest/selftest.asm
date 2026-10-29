; ══════════════════════════════════════════════════════════
;  Selftest — the device checks what the assembler writes
;
;  Each check computes a number with the instructions it is about
;  and compares it with the number it should be.  The screen says
;  how many held, and the numbers of those that did not:
;
;      selftest 18/18
;
;  It covers what a reader of the bytes cannot run: the switches and
;  fill-array-data with their payloads, a try range with a typed
;  catch and one with a catch-all, and the narrower and wider forms
;  of constants, shifts, array and field access.  A dex the verifier
;  refuses does not start at all, which is a check of its own.
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

; v0 holds what a check computed; v10 counts the checks, v11 those
; that held, v12 collects the numbers of those that did not.
macro check? n?*, expected?*
    const      v1, expected
    if_eq      v0, v1, .ok_#n
    const_16   v2, n
    invoke_virtual "Ljava/lang/StringBuilder;->append(I)Ljava/lang/StringBuilder;", v12, v2
    const_string v2, " "
    invoke_virtual "Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;", v12, v2
    goto       .next_#n
  .ok_#n:
    add_int_lit8 v11, v11, 1
  .next_#n:
    add_int_lit8 v10, v10, 1
end macro

macro _class
    defclass "Lapp/hello/selftest/Main;", ACC_PUBLIC, "Landroid/app/Activity;"
    deffield "->w:J", ACC_PUBLIC or ACC_STATIC

    method "-><init>()V", ACC_PUBLIC or ACC_CONSTRUCTOR, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    ;  A frame of 300 registers, so that move/16 has somewhere to go
    ;  that no byte can name.  Registers have names up to v255, and
    ;  one past them is written from v0.
    method "->far(I)I", ACC_PUBLIC or ACC_STATIC, locals 300
        move_16        v0 + 280, p0
        move_16        v0, v0 + 280
        return         v0
    end method

    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 13
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1
        const_4        v10, 0
        const_4        v11, 0
        new_instance   v12, "Ljava/lang/StringBuilder;"
        invoke_direct  "Ljava/lang/StringBuilder;-><init>()V", v12

        ; 1: packed-switch picks the second of three cases
        const_16       v3, 11
        const_16       v0, -1
        packed_switch  v3, 10, .p10, .p11, .p12
        goto           .p_end
    .p10:
        const_16       v0, 10
        goto           .p_end
    .p11:
        const_16       v0, 111
        goto           .p_end
    .p12:
        const_16       v0, 12
    .p_end:
        check 1, 111

        ; 2: sparse-switch on a negative key, cases written out of order
        const_16       v3, -5
        const_16       v0, -1
        sparse_switch  v3, 100, .s100, -5, .sneg, 7, .s7
        goto           .s_end
    .s100:
        const_16       v0, 100
        goto           .s_end
    .sneg:
        const_16       v0, 55
        goto           .s_end
    .s7:
        const_16       v0, 7
    .s_end:
        check 2, 55

        ; 3: fill-array-data into an int[], summed
        const_4        v5, 3
        new_array      v4, v5, "[I"
        fill_array_data v4, 4, 1000, -2000, 3
        const_4        v5, 0
        aget           v0, v4, v5
        const_4        v5, 1
        aget           v6, v4, v5
        add_int        v0, v0, v6
        const_4        v5, 2
        aget           v6, v4, v5
        add_int        v0, v0, v6
        check 3, -997

        ; 4: into a byte[], read back signed
        const_4        v5, 3
        new_array      v4, v5, "[B"
        fill_array_data v4, 1, 1, -2, 3
        const_4        v5, 1
        aget_byte      v0, v4, v5
        check 4, -2

        ; 5: into a long[], the high word and the low word
        const_4        v5, 2
        new_array      v4, v5, "[J"
        fill_array_data v4, 8, $123456789A, -1
        const_4        v5, 0
        aget_wide      v6, v4, v5
        long_to_int    v0, v6
        check 5, $3456789A
        const_16       v3, 32
        shr_long       v6, v6, v3
        long_to_int    v0, v6
        check 6, $12

        ; 7: aput-wide and back
        const_wide_16  v6, -9
        const_4        v5, 1
        aput_wide      v6, v4, v5
        aget_wide      v8, v4, v5
        long_to_int    v0, v8
        check 7, -9

        ; 8: a typed catch: parseInt("x") throws NumberFormatException
        const_string   v3, "x"
        const_16       v0, -1
    .t1:
        invoke_static  "Ljava/lang/Integer;->parseInt(Ljava/lang/String;)I", v3
        move_result    v0
    .t1_end:
        goto           .t1_done
    .h1:
        move_exception v2
        const_16       v0, 77
    .t1_done:
        check 8, 77

        ; 9: a catch-all: 1 / 0
        const_4        v3, 1
        const_4        v5, 0
        const_16       v0, -1
    .t2:
        div_int        v0, v3, v5
    .t2_end:
        goto           .t2_done
    .h2:
        move_exception v2
        const_16       v0, 88
    .t2_done:
        check 9, 88

        ; 10: instance-of
        const_string   v3, "s"
        instance_of    v0, v3, "Ljava/lang/String;"
        check 10, 1

        ; 11: filled-new-array, the middle element
        const_4        v3, 5
        const_4        v5, 6
        const_4        v6, 7
        filled_new_array "[I", v3, v5, v6
        move_result_object v4
        const_4        v5, 1
        aget           v0, v4, v5
        check 11, 6

        ; 12: lit16, and rsub-int
        const_16       v3, 7
        add_int_lit16  v0, v3, 1000
        rsub_int       v0, v0, 2000
        check 12, 993

        ; 13: const/high16 is 1.0f
        const_high16   v3, $3F800000
        float_to_int   v0, v3
        check 13, 1

        ; 14: const-wide/16, /32 and /high16, the last 2.0
        const_wide_16  v6, -3
        long_to_int    v0, v6
        check 14, -3
        const_wide_32  v6, -100000
        long_to_int    v0, v6
        check 15, -100000
        const_wide_high16 v6, $4000000000000000
        double_to_int  v0, v6
        check 16, 2

        ; 17: shl-long by an int, then ushr-long
        const_wide_16  v6, 1
        const_16       v3, 40
        shl_long       v6, v6, v3
        const_16       v3, 39
        ushr_long      v6, v6, v3
        long_to_int    v0, v6
        check 17, 2

        ; 18: sput-wide and sget-wide
        const_wide_32  v6, 123456
        sput_wide      v6, "->w:J"
        const_wide_16  v6, 0
        sget_wide      v6, "->w:J"
        long_to_int    v0, v6
        check 18, 123456

        ; 19: move/16 through register 280 and back
        const_16       v3, 42
        invoke_static  "->far(I)I", v3
        move_result    v0
        check 19, 42

        ; 20: const-string/jumbo
        const_string_jumbo v3, "jumbo"
        invoke_virtual "Ljava/lang/String;->length()I", v3
        move_result    v0
        check 20, 5

        ; what the screen says
        new_instance   v4, "Ljava/lang/StringBuilder;"
        invoke_direct  "Ljava/lang/StringBuilder;-><init>()V", v4
        const_string   v3, "selftest "
        invoke_virtual "Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;", v4, v3
        invoke_virtual "Ljava/lang/StringBuilder;->append(I)Ljava/lang/StringBuilder;", v4, v11
        const_string   v3, "/"
        invoke_virtual "Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;", v4, v3
        invoke_virtual "Ljava/lang/StringBuilder;->append(I)Ljava/lang/StringBuilder;", v4, v10
        if_eq          v10, v11, .all
        const_string   v3, "\nfailed: "
        invoke_virtual "Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;", v4, v3
        invoke_virtual "Ljava/lang/StringBuilder;->append(Ljava/lang/CharSequence;)Ljava/lang/StringBuilder;", v4, v12
    .all:
        invoke_virtual "Ljava/lang/StringBuilder;->toString()Ljava/lang/String;", v4
        move_result_object v3

        new_instance   v5, "Landroid/widget/TextView;"
        invoke_direct  "Landroid/widget/TextView;-><init>(Landroid/content/Context;)V", v5, p0
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v5, v3
        const_high16   v3, $42000000
        invoke_virtual "Landroid/widget/TextView;->setTextSize(F)V", v5, v3
        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v5
        return_void

        catch     "Ljava/lang/NumberFormatException;", .t1, .t1_end, .h1
        catch_all .t2, .t2_end, .h2
    end method
end macro

macro _types
    deftype "Ljava/lang/Object;"
end macro

dex_lists _class, _types

dex_file
