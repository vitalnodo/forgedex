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
    defstr _init,        "<init>"
    defstr _I,           "I"
    defstr _L,           "L"
    defstr _LI,          "LI"
    defstr _activity,    "Landroid/app/Activity;"
    defstr _context,     "Landroid/content/Context;"
    defstr _bundle,      "Landroid/os/Bundle;"
    defstr _view,        "Landroid/view/View;"
    defstr _textview,    "Landroid/widget/TextView;"
    defstr _hello,       "Lapp/hello/jni/HelloWorld;"
    defstr _charseq,     "Ljava/lang/CharSequence;"
    defstr _integer,     "Ljava/lang/Integer;"
    defstr _object,      "Ljava/lang/Object;"
    defstr _string,      "Ljava/lang/String;"
    defstr _system,      "Ljava/lang/System;"
    defstr _V,           "V"
    defstr _VL,          "VL"
    defstr _answer,      "answer"
    defstr _hello_lib,   "hello"
    defstr _loadlibrary, "loadLibrary"
    defstr _oncreate,    "onCreate"
    defstr _setcv,       "setContentView"
    defstr _settext,     "setText"
    defstr _tostring,    "toString"
end macro

postpone
__global::

    dex_header

    string_ids:
        emit_string_ids _strings

    ; ── type_ids ──────────────────────────────────────────
    ; Sorted by string_idx ascending
    type_ids:
        _I_type        type_id_item _I_string          ; [0]  int
        _activity_type type_id_item _activity_string   ; [1]  Activity
        _context_type  type_id_item _context_string    ; [2]  Context
        _bundle_type   type_id_item _bundle_string     ; [3]  Bundle
        _view_type     type_id_item _view_string       ; [4]  View
        _textview_type type_id_item _textview_string   ; [5]  TextView
        _hello_type    type_id_item _hello_string      ; [6]  HelloWorld
        _charseq_type  type_id_item _charseq_string    ; [7]  CharSequence
        _integer_type  type_id_item _integer_string    ; [8]  Integer
        _object_type   type_id_item _object_string     ; [9]  Object
        _string_type   type_id_item _string_string     ; [10] String
        _system_type   type_id_item _system_string     ; [11] System
        _void_type     type_id_item _V_string          ; [12] V

    ; ── proto_ids ─────────────────────────────────────────
    proto_ids:
        _I_proto          proto_id_item _I_string,  _I_type,      $00          ; [0] ()I
        _string_ret_proto proto_id_item _L_string,  _string_type, $00          ; [1] ()String
        _I_str_proto      proto_id_item _LI_string, _string_type, _I_tl        ; [2] (I)String
        _void_proto       proto_id_item _V_string,  _void_type,   $00          ; [3] ()V
        _context_proto    proto_id_item _VL_string, _void_type,   _context_tl  ; [4] (Context)V
        _bundle_proto     proto_id_item _VL_string, _void_type,   _bundle_tl   ; [5] (Bundle)V
        _view_proto       proto_id_item _VL_string, _void_type,   _view_tl     ; [6] (View)V
        _charseq_proto    proto_id_item _VL_string, _void_type,   _charseq_tl  ; [7] (CharSequence)V
        _string_proto     proto_id_item _VL_string, _void_type,   _string_tl   ; [8] (String)V

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _act_init_m    method_id_item _activity_type, _void_proto,       _init_string        ; [0]
        _act_oncr_m    method_id_item _activity_type, _bundle_proto,     _oncreate_string    ; [1]
        _act_setcv_m   method_id_item _activity_type, _view_proto,       _setcv_string       ; [2]
        _tv_init_m     method_id_item _textview_type, _context_proto,    _init_string        ; [3]
        _tv_settt_m    method_id_item _textview_type, _charseq_proto,    _settext_string     ; [4]
        _hw_init_m     method_id_item _hello_type,    _void_proto,       _init_string        ; [5]
        _hw_answer_m   method_id_item _hello_type,    _string_ret_proto, _answer_string      ; [6]
        _hw_oncr_m     method_id_item _hello_type,    _bundle_proto,     _oncreate_string    ; [7]
        _int_tos_m     method_id_item _integer_type,  _I_str_proto,      _tostring_string    ; [8]
        _sys_loadlib_m method_id_item _system_type,   _string_proto,     _loadlibrary_string ; [9]

    class_defs:
        _hello_class class_def_item \
            _hello_type, ACC_PUBLIC, _activity_type, \
            $00, NO_INDEX, $00, _hello_class_data, $00

    data:
        data_strings:
            emit_string_data _strings

        align $04

        data_type_lists:
            _I_tl       type_list $01, <_I_type>
            _context_tl type_list $01, <_context_type>
            _bundle_tl  type_list $01, <_bundle_type>
            _view_tl    type_list $01, <_view_type>
            _charseq_tl type_list $01, <_charseq_type>
            _string_tl  type_list $01, <_string_type>

        align $04

        ; static=0, instance=0, direct=1(<init>), virtual=2(answer[native],onCreate)
        data_class_data:
            _hello_class_data class_data_item $00, $00, $01, $02, \
                <>, <>, \
                <<_hw_init_m,   ACC_PUBLIC or ACC_CONSTRUCTOR, _hw_init_code>>, \
                <<_hw_answer_m, ACC_PUBLIC or ACC_NATIVE,      $00>, \
                 <$01,          ACC_PUBLIC,                    _hw_oncreate_code>>

        align $04

        ; <init>()V: registers=1, ins=1(v0=this), outs=1, 4 code units
        _hw_init_code code_item $01, $01, $01, $00, $00, $04, _init_insns

        align $04

        ; onCreate(Bundle)V: registers=4, ins=2(v2=this, v3=bundle), outs=2, 24 code units
        _hw_oncreate_code code_item $04, $02, $02, $00, $00, $18, _oncreate_insns

        align $04

        data_map:
            _map map_list $0B, <\
                <TYPE_HEADER_ITEM,      $01,             header>,\
                <TYPE_STRING_ID_ITEM,   string_ids_size, string_ids>,\
                <TYPE_TYPE_ID_ITEM,     type_ids_size,   type_ids>,\
                <TYPE_PROTO_ID_ITEM,    proto_ids_size,  proto_ids>,\
                <TYPE_METHOD_ID_ITEM,   method_ids_size, method_ids>,\
                <TYPE_CLASS_DEF_ITEM,   class_defs_size, class_defs>,\
                <TYPE_STRING_DATA_ITEM, $19,             data_strings>,\
                <TYPE_TYPE_LIST,        $06,             data_type_lists>,\
                <TYPE_CLASS_DATA_ITEM,  $01,             data_class_data>,\
                <TYPE_CODE_ITEM,        $02,             _hw_init_code>,\
                <TYPE_MAP_LIST,         $01,             data_map>\
            >

        align $04

    dex_footer

end postpone

; ── <init>()V ─────────────────────────────────────────────
; v0 = this (p0)
virtual at $00
_init_insns::
    invoke_direct  _act_init_m, v0                      ; Activity.<init>
    return_void
end virtual

; ── onCreate(Bundle)V ─────────────────────────────────────
; registers=4, ins=2: v2=this(p0), v3=bundle(p1)
; v0=temp/result, v1=TextView
virtual at $00
_oncreate_insns::
    invoke_super   _act_oncr_m, v2, v3                  ; Activity.onCreate
    const_string   v0, _hello_lib_string                   ; "hello"
    invoke_static  _sys_loadlib_m, v0                   ; System.loadLibrary
    new_instance   v1, _textview_type                   ; new TextView
    invoke_direct  _tv_init_m, v1, v2                   ; TextView.<init>
    invoke_virtual _hw_answer_m, v2                     ; this.answer()
    move_result_object v0                               ; result
    invoke_virtual _tv_settt_m, v1, v0                  ; setText
    invoke_virtual _act_setcv_m, v2, v1                 ; setContentView
    return_void
end virtual
