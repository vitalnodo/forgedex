format binary as "dex"
org $00

include '../../forgedex.inc'

macro _strings
    defstr _init,            "<init>"
    defstr _hello_world,     "Hello, World!"
    defstr _activity,        "Landroid/app/Activity;"
    defstr _context,         "Landroid/content/Context;"
    defstr _bundle,          "Landroid/os/Bundle;"
    defstr _view,            "Landroid/view/View;"
    defstr _textview,        "Landroid/widget/TextView;"
    defstr _hello,           "Lapp/hello/HelloWorld;"
    defstr _charsequence,    "Ljava/lang/CharSequence;"
    defstr _object,          "Ljava/lang/Object;"
    defstr _V,               "V"
    defstr _shorty_vl,       "VL"
    defstr _oncreate,        "onCreate"
    defstr _setcontentview,  "setContentView"
    defstr _settext,         "setText"
end macro

postpone
__global::

    dex_header

    string_ids:
        emit_string_ids _strings

    type_ids:
        deftype _activity
        deftype _context
        deftype _bundle
        deftype _view
        deftype _textview
        deftype _hello
        deftype _charsequence
        deftype _object
        deftype _V

    ; ── proto_ids ─────────────────────────────────────────
    ; All return void (type[8]), sorted by param type_idx: none < 1 < 2 < 3 < 6
    proto_ids:
        _void_v_proto     proto_id_item _V_string,         _V_type, $00             ; [0] ()V
        _context_v_proto  proto_id_item _shorty_vl_string, _V_type, _context_v_tl   ; [1] (Context)V
        _bundle_v_proto   proto_id_item _shorty_vl_string, _V_type, _bundle_v_tl    ; [2] (Bundle)V
        _view_v_proto     proto_id_item _shorty_vl_string, _V_type, _view_v_tl      ; [3] (View)V
        _charseq_v_proto  proto_id_item _shorty_vl_string, _V_type, _charseq_v_tl   ; [4] (CharSequence)V

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _activity_init_method     method_id_item _activity_type, _void_v_proto,    _init_string           ; [0]
        _activity_oncreate_method method_id_item _activity_type, _bundle_v_proto,  _oncreate_string       ; [1]
        _activity_setcv_method    method_id_item _activity_type, _view_v_proto,    _setcontentview_string ; [2]
        _textview_init_method     method_id_item _textview_type, _context_v_proto, _init_string           ; [3]
        _textview_settext_method  method_id_item _textview_type, _charseq_v_proto, _settext_string        ; [4]
        _hello_init_method        method_id_item _hello_type,    _void_v_proto,    _init_string           ; [5]
        _hello_oncreate_method    method_id_item _hello_type,    _bundle_v_proto,  _oncreate_string       ; [6]

    class_defs:
        _hello_class class_def_item \
            _hello_type, ACC_PUBLIC, _activity_type, \
            $00, NO_INDEX, $00, _hello_class_data, $00

    data:
        data_strings:
            emit_string_data _strings

        align $04

        data_type_lists:
            _context_v_tl  type_list $01, <_context_type>
            _bundle_v_tl   type_list $01, <_bundle_type>
            _view_v_tl     type_list $01, <_view_type>
            _charseq_v_tl  type_list $01, <_charsequence_type>

        align $04

        ; static=0, instance=0, direct=1(<init>), virtual=1(onCreate)
        data_class_data:
            _hello_class_data class_data_item $00, $00, $01, $01, \
                <>, <>, \
                <<_hello_init_method,     ACC_PUBLIC, _hello_init_code>>, \
                <<_hello_oncreate_method, ACC_PUBLIC, _hello_oncreate_code>>

        align $04

        ; <init>()V: registers=1, ins=1(v0=this), outs=1, 4 code units
        data_init_code:
            _hello_init_code code_item $01, $01, $01, $00, $00, $04, _hello_init_insns

        align $04

        ; onCreate(Bundle)V: registers=4, ins=2(v2=this, v3=bundle), outs=2, 17 code units
        data_oncreate_code:
            _hello_oncreate_code code_item $04, $02, $02, $00, $00, $11, _hello_oncreate_insns

        align $04

        data_map:
            _map map_list $0B, <\
                <TYPE_HEADER_ITEM,      $01,             header>,\
                <TYPE_STRING_ID_ITEM,   string_ids_size, string_ids>,\
                <TYPE_TYPE_ID_ITEM,     type_ids_size,   type_ids>,\
                <TYPE_PROTO_ID_ITEM,    proto_ids_size,  proto_ids>,\
                <TYPE_METHOD_ID_ITEM,   method_ids_size, method_ids>,\
                <TYPE_CLASS_DEF_ITEM,   class_defs_size, class_defs>,\
                <TYPE_STRING_DATA_ITEM, $0F,             data_strings>,\
                <TYPE_TYPE_LIST,        $04,             data_type_lists>,\
                <TYPE_CLASS_DATA_ITEM,  $01,             data_class_data>,\
                <TYPE_CODE_ITEM,        $02,             data_init_code>,\
                <TYPE_MAP_LIST,         $01,             data_map>\
            >

        align $04

    dex_footer

end postpone

; ── <init>()V ─────────────────────────────────────────────
; v0 = this (p0)
virtual at $00
_hello_init_insns::
    invoke_direct  _activity_init_method, v0            ; Activity.<init>
    return_void
end virtual

; ── onCreate(Bundle)V ─────────────────────────────────────
; registers=4, ins=2: v2=this(p0), v3=bundle(p1)
; v0=TextView, v1="Hello, World!"
virtual at $00
_hello_oncreate_insns::
    invoke_super   _activity_oncreate_method, v2, v3    ; Activity.onCreate
    new_instance   v0, _textview_type                   ; new TextView
    invoke_direct  _textview_init_method, v0, v2        ; TextView.<init>
    const_string   v1, _hello_world_string              ; "Hello, World!"
    invoke_virtual _textview_settext_method, v0, v1     ; setText
    invoke_virtual _activity_setcv_method, v2, v0       ; setContentView
    return_void
end virtual
