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

macro _strings
    defstr     _init,        "<init>"
    defstrtype _I,           "I"
    defstr     _L,           "L"
    defstr     _LI,          "LI"
    defstrtype _activity,    "Landroid/app/Activity;"
    defstrtype _context,     "Landroid/content/Context;"
    defstrtype _bundle,      "Landroid/os/Bundle;"
    defstrtype _view,        "Landroid/view/View;"
    defstrtype _textview,    "Landroid/widget/TextView;"
    defstrtype _hello,       "Lapp/hello/jni/HelloWorld;"
    defstrtype _charseq,     "Ljava/lang/CharSequence;"
    defstrtype _integer,     "Ljava/lang/Integer;"
    defstrtype _object,      "Ljava/lang/Object;"
    defstrtype _string,      "Ljava/lang/String;"
    defstrtype _system,      "Ljava/lang/System;"
    defstrtype _V,           "V"
    defstr     _VL,          "VL"
    defstr     _answer,      "answer"
    defstr     _hello_lib,   "hello"
    defstr     _loadlibrary, "loadLibrary"
    defstr     _oncreate,    "onCreate"
    defstr     _setcv,       "setContentView"
    defstr     _settext,     "setText"
    defstr     _tostring,    "toString"
end macro

postpone
__global::

    dex_header

    string_ids: emit_string_ids _strings
    type_ids:   emit_type_ids   _strings

    ; ── proto_ids ─────────────────────────────────────────
    proto_ids:
        _I_proto          proto _I, _I,      $00          ; [0] ()I
        _string_ret_proto proto _L, _string, $00          ; [1] ()String
        _I_str_proto      proto _LI, _string, _I_tl        ; [2] (I)String
        _void_proto       proto _V, _V,   $00          ; [3] ()V
        _context_proto    proto _VL, _V,   _context_tl  ; [4] (Context)V
        _bundle_proto     proto _VL, _V,   _bundle_tl   ; [5] (Bundle)V
        _view_proto       proto _VL, _V,   _view_tl     ; [6] (View)V
        _charseq_proto    proto _VL, _V,   _charseq_tl  ; [7] (CharSequence)V
        _string_proto     proto _VL, _V,   _string_tl   ; [8] (String)V

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _act_init_m    method_id _activity_type, _void_proto, _init        ; [0]
        _act_oncr_m    method_id _activity_type, _bundle_proto, _oncreate    ; [1]
        _act_setcv_m   method_id _activity_type, _view_proto, _setcv       ; [2]
        _tv_init_m     method_id _textview_type, _context_proto, _init        ; [3]
        _tv_settt_m    method_id _textview_type, _charseq_proto, _settext     ; [4]
        _hw_answer_m   method_id _hello_type,    _string_ret_proto, _answer   ; [5]
        _hw_init_m     method_id _hello_type,    _void_proto,       _init     ; [6]
        _hw_oncr_m     method_id _hello_type,    _bundle_proto,     _oncreate ; [7]
        _int_tos_m     method_id _integer_type,  _I_str_proto, _tostring    ; [8]
        _sys_loadlib_m method_id _system_type,   _string_proto, _loadlibrary ; [9]

    class_defs:
        _hello_class public_class _hello_type, _activity_type, $00, _hello_class_data

    data:
        data_strings _strings


        data_type_lists
            _I_tl       type_list <_I>
            _context_tl type_list <_context>
            _bundle_tl  type_list <_bundle>
            _view_tl    type_list <_view>
            _charseq_tl type_list <_charseq>
            _string_tl  type_list <_string>


        ; static=0, instance=0, direct=1(<init>), virtual=2(answer[native],onCreate)
        data_class_data
            _hello_class_data class_data_item \
                <>, <>, \
                <<_hw_init_m,   ACC_PUBLIC or ACC_CONSTRUCTOR, _hw_init_code>>, \
                <<_hw_answer_m, ACC_PUBLIC or ACC_NATIVE,      $00>, \
                 <_hw_oncr_m,   ACC_PUBLIC,                    _hw_oncreate_code>>


        data_code
        ; ── <init>()V ─────────────────────────────────────────────
        ; p0 = this
        dex_method _hw_init_code, $01, $01, $01
            invoke_direct  _act_init_m, p0                      ; Activity.<init>
            return_void
        end dex_method

        ; ── onCreate(Bundle)V ─────────────────────────────────────
        ; registers=4, ins=2: p0=this, p1=bundle
        ; v0=temp/result, v1=TextView
        dex_method _hw_oncreate_code, $04, $02, $02
            invoke_super   _act_oncr_m, p0, p1                  ; Activity.onCreate
            const_string   v0, _hello_lib_string                ; "hello"
            invoke_static  _sys_loadlib_m, v0                   ; System.loadLibrary
            new_instance   v1, _textview_type                   ; new TextView
            invoke_direct  _tv_init_m, v1, p0                   ; TextView.<init>
            invoke_virtual _hw_answer_m, p0                     ; this.answer()
            move_result_object v0                               ; result
            invoke_virtual _tv_settt_m, v1, v0                  ; setText
            invoke_virtual _act_setcv_m, p0, v1                 ; setContentView
            return_void
        end dex_method

        dex_map_list

    dex_footer

end postpone
