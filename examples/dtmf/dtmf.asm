; ══════════════════════════════════════════════════════════
;  DTMF — a keypad, and the tones a telephone would send
;
;  class Dialer extends Activity implements View.OnClickListener {
;      byte[] bin;          // dtmf.bin, read out of the APK
;      AudioTrack track;
;      TextView display;
;      String dialed;
;
;      static native short[] tone(byte[] bin, String digit);
;
;      public void onCreate(Bundle b) { ...a TextView over 4x3 buttons... }
;
;      public void onClick(View v) {
;          String d = "123456789*0#".substring(v.getId(), v.getId() + 1);
;          track.write(tone(bin, d), 0, ...);
;          display.setText(dialed += d);
;      }
;  }
;
;  Each key is two sine waves at once — one for its row, one for
;  its column — which is what "dual tone" means and why a keypad
;  needs only eight frequencies to name twelve keys:
;
;           1209  1336  1477 Hz
;          ┌─────┬─────┬─────┐
;    697   │  1  │  2  │  3  │
;          ├─────┼─────┼─────┤
;    770   │  4  │  5  │  6  │
;          ├─────┼─────┼─────┤
;    852   │  7  │  8  │  9  │
;          ├─────┼─────┼─────┤
;    941   │  *  │  0  │  #  │
;          └─────┴─────┴─────┘
;
;  Not one sample of that is computed here, or in any C written
;  for this repository.  The tones come from the winning IOCCC
;  2020 entry by Edward Giles, whose source is laid out in the
;  shape of a telephone handset, and which is in ioccc/ exactly as
;  it was submitted.  It is a command-line program that reads and
;  writes WAV files, so what stands between it and this Activity
;  is shim/ — fifteen functions of <stdio.h> and <string.h>
;  reimplemented over plain memory, which is all the entry ever
;  needed.  It never learns that it is inside an APK.
;
;  dtmf.bin is the entry's own data file: a WAV header to copy,
;  the oscillator constants for the eight frequencies, and the key
;  table.  It rides along in assets/, which needs no aapt and no
;  resource ids — the Dalvik side below opens it by name.
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _types
    deftype "Ljava/lang/Object;"
end macro

macro _class
    defclass "Lapp/hello/dtmf/Dialer;", ACC_PUBLIC, \
        "Landroid/app/Activity;", _dtmf_ifaces

    deffield "->bin:[B", ACC_PUBLIC
    deffield "->track:Landroid/media/AudioTrack;", ACC_PUBLIC
    deffield "->display:Landroid/widget/TextView;", ACC_PUBLIC
    deffield "->dialed:Ljava/lang/String;", ACC_PUBLIC

    ; Bound from JNI_OnLoad rather than found by name, so the C
    ; side names it plainly and not Java_app_hello_dtmf_Dialer_tone.
    defmethod "->tone([BLjava/lang/String;)[S", \
        ACC_PUBLIC or ACC_STATIC or ACC_NATIVE

    method "-><init>()V", ACC_PUBLIC or ACC_CONSTRUCTOR, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    ; v0=root/scratch  v1=row/stream  v2=key  v3=button  v4..v7=scratch
    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 8
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1

        const_string   v0, "dtmf"
        invoke_static  "Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V", v0

        ; ── dtmf.bin, straight out of assets/ ─────────────
        ;
        ;  available() gives the whole length for an asset, so the
        ;  file comes back in one read; it is 253 bytes.
        invoke_virtual "Landroid/app/Activity;->getAssets()Landroid/content/res/AssetManager;", p0
        move_result_object v0
        const_string   v1, "dtmf.bin"
        invoke_virtual "Landroid/content/res/AssetManager;->open(Ljava/lang/String;)Ljava/io/InputStream;", v0, v1
        move_result_object v1
        invoke_virtual "Ljava/io/InputStream;->available()I", v1
        move_result    v4
        new_array      v3, v4, "[B"
        invoke_virtual "Ljava/io/InputStream;->read([B)I", v1, v3
        invoke_virtual "Ljava/io/InputStream;->close()V", v1
        iput_object    v3, p0, "->bin:[B"

        const_string   v0, ""
        iput_object    v0, p0, "->dialed:Ljava/lang/String;"

        ; ── One AudioTrack, refilled and replayed per key ─────
        ;
        ;  The entry emits 16 kHz mono, 3200 samples a digit — a
        ;  fifth of a second, or 6400 bytes, which is exactly the
        ;  buffer asked for here.
        ;
        ;  MODE_STATIC, not MODE_STREAM.  A streaming track sized
        ;  for two seconds of audio underruns between taps and has
        ;  to be restarted on the next write, and that restart is
        ;  heard as a delay before the tone.  A static track holds
        ;  the whole tone and starts on play().
        new_instance   v0, "Landroid/media/AudioTrack;"
        const_4        v1, 3                          ; STREAM_MUSIC
        const          v2, 16000
        const_4        v3, 4                          ; CHANNEL_OUT_MONO
        const_4        v4, 2                          ; ENCODING_PCM_16BIT
        const          v5, 6400
        const_4        v6, 0                          ; MODE_STATIC
        invoke_direct_range "Landroid/media/AudioTrack;-><init>(IIIIII)V", v0, v6

        ;  A static track comes out of its constructor holding no
        ;  data, and in that state it refuses stop().  One buffer
        ;  of silence puts it in the state the first tap expects.
        const          v1, 3200
        new_array      v2, v1, "[S"
        const_4        v3, 0
        invoke_virtual "Landroid/media/AudioTrack;->write([SII)I", v0, v2, v3, v1

        iput_object    v0, p0, "->track:Landroid/media/AudioTrack;"

        ; ── The screen: a display over four rows of three ─────
        new_instance   v0, "Landroid/widget/LinearLayout;"
        invoke_direct  "Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V", v0, p0
        const_4        v4, 1                          ; VERTICAL
        invoke_virtual "Landroid/widget/LinearLayout;->setOrientation(I)V", v0, v4

        new_instance   v3, "Landroid/widget/TextView;"
        invoke_direct  "Landroid/widget/TextView;-><init>(Landroid/content/Context;)V", v3, p0
        const_float    v4, 36.0
        invoke_virtual "Landroid/widget/TextView;->setTextSize(F)V", v3, v4
        iput_object    v3, p0, "->display:Landroid/widget/TextView;"

        new_instance   v5, "Landroid/widget/LinearLayout$LayoutParams;"
        const_4        v6, -1                         ; MATCH_PARENT
        const_4        v7, 0
        const_float    v4, 1.0
        invoke_direct  "Landroid/widget/LinearLayout$LayoutParams;-><init>(IIF)V", v5, v6, v7, v4
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V", v0, v3, v5

        ;  Twelve keys in one loop.  Every third opens a row, and
        ;  the label is a slice of one string rather than twelve
        ;  of them: "123456789*0#".substring(k, k + 1).
        ;
        ;  v1 is the row, and starts out holding the root rather
        ;  than nothing.  The two paths into .same_row meet there,
        ;  and the verifier types the register by what they have
        ;  in common: another LinearLayout keeps it a LinearLayout,
        ;  where anything else would widen it to View and take
        ;  addView away.
        move_object    v1, v0
        const_4        v2, 0
    .key:
        const          v4, 12
        if_ge          v2, v4, .done

        rem_int_lit8   v4, v2, 3
        if_nez         v4, .same_row
        new_instance   v1, "Landroid/widget/LinearLayout;"
        invoke_direct  "Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V", v1, p0
        const_4        v4, 0                          ; HORIZONTAL
        invoke_virtual "Landroid/widget/LinearLayout;->setOrientation(I)V", v1, v4
        new_instance   v5, "Landroid/widget/LinearLayout$LayoutParams;"
        const_4        v6, -1
        const_4        v7, 0
        const_float    v4, 2.0
        invoke_direct  "Landroid/widget/LinearLayout$LayoutParams;-><init>(IIF)V", v5, v6, v7, v4
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V", v0, v1, v5
    .same_row:

        new_instance   v3, "Landroid/widget/Button;"
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v3, p0

        const_string   v4, "123456789*0#"
        add_int_lit8   v5, v2, 1
        invoke_virtual "Ljava/lang/String;->substring(II)Ljava/lang/String;", v4, v2, v5
        move_result_object v4
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v3, v4

        invoke_virtual "Landroid/view/View;->setId(I)V", v3, v2
        invoke_virtual "Landroid/view/View;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v3, p0

        new_instance   v5, "Landroid/widget/LinearLayout$LayoutParams;"
        const_4        v6, 0
        const_4        v7, -1
        const_float    v4, 1.0
        invoke_direct  "Landroid/widget/LinearLayout$LayoutParams;-><init>(IIF)V", v5, v6, v7, v4
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V", v1, v3, v5

        add_int_lit8   v2, v2, 1
        goto           .key
    .done:

        ;  One more key under the rest, carrying the id one past
        ;  the twelve.  onClick tells it from a digit by that.
        new_instance   v3, "Landroid/widget/Button;"
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v3, p0
        const_string   v4, "clear"
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v3, v4
        const          v4, 12
        invoke_virtual "Landroid/view/View;->setId(I)V", v3, v4
        invoke_virtual "Landroid/view/View;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v3, p0

        new_instance   v5, "Landroid/widget/LinearLayout$LayoutParams;"
        const_4        v6, -1
        const_4        v7, 0
        const_float    v4, 1.0
        invoke_direct  "Landroid/widget/LinearLayout$LayoutParams;-><init>(IIF)V", v5, v6, v7, v4
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V", v0, v3, v5

        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v0
        return_void
    end method

    ; v0=key  v1=digit  v2=samples  v3=track/text  v4=0  v5=length
    method "->onClick(Landroid/view/View;)V", ACC_PUBLIC, locals 6
        invoke_virtual "Landroid/view/View;->getId()I", p1
        move_result    v0

        ;  The twelve keys carry their own index; clear carries 12.
        const          v1, 12
        if_lt          v0, v1, .digit

        const_string   v1, ""
        iput_object    v1, p0, "->dialed:Ljava/lang/String;"
        iget_object    v2, p0, "->display:Landroid/widget/TextView;"
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v2, v1
        return_void
    .digit:

        const_string   v1, "123456789*0#"
        add_int_lit8   v2, v0, 1
        invoke_virtual "Ljava/lang/String;->substring(II)Ljava/lang/String;", v1, v0, v2
        move_result_object v1

        iget_object    v2, p0, "->bin:[B"
        invoke_static  "Lapp/hello/dtmf/Dialer;->tone([BLjava/lang/String;)[S", v2, v1
        move_result_object v2

        ;  Stop before refilling: a static track only takes new
        ;  samples once it is not playing the old ones.
        iget_object    v3, p0, "->track:Landroid/media/AudioTrack;"
        invoke_virtual "Landroid/media/AudioTrack;->stop()V", v3
        const_4        v4, 0
        array_length   v5, v2
        invoke_virtual "Landroid/media/AudioTrack;->write([SII)I", v3, v2, v4, v5
        invoke_virtual "Landroid/media/AudioTrack;->play()V", v3

        iget_object    v3, p0, "->dialed:Ljava/lang/String;"
        invoke_virtual "Ljava/lang/String;->concat(Ljava/lang/String;)Ljava/lang/String;", v3, v1
        move_result_object v3
        iput_object    v3, p0, "->dialed:Ljava/lang/String;"
        iget_object    v4, p0, "->display:Landroid/widget/TextView;"
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v4, v3

        return_void
    end method
end macro

macro _interfaces
    _dtmf_ifaces deftypelist "Landroid/view/View$OnClickListener;"
end macro

dex_lists _class, _types

dex_file _interfaces
