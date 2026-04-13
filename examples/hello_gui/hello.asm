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

    proto_ids:
        _void_v_proto     proto_id_item _V_string,         _V_type, $00
        _context_v_proto  proto_id_item _shorty_vl_string, _V_type, _context_v_tl
        _bundle_v_proto   proto_id_item _shorty_vl_string, _V_type, _bundle_v_tl
        _view_v_proto     proto_id_item _shorty_vl_string, _V_type, _view_v_tl
        _charseq_v_proto  proto_id_item _shorty_vl_string, _V_type, _charseq_v_tl

    method_ids:
        _activity_init_method     method_id_item _activity_type, _void_v_proto,    _init_string
        _activity_oncreate_method method_id_item _activity_type, _bundle_v_proto,  _oncreate_string
        _activity_setcv_method    method_id_item _activity_type, _view_v_proto,    _setcontentview_string
        _textview_init_method     method_id_item _textview_type, _context_v_proto, _init_string
        _textview_settext_method  method_id_item _textview_type, _charseq_v_proto, _settext_string
        _hello_init_method        method_id_item _hello_type,    _void_v_proto,    _init_string
        _hello_oncreate_method    method_id_item _hello_type,    _bundle_v_proto,  _oncreate_string

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

        data_class_data:
            _hello_class_data class_data_item $00, $00, $01, $01, \
                <>, <>, \
                <<_hello_init_method,     ACC_PUBLIC, _hello_init_code>>, \
                <<_hello_oncreate_method, ACC_PUBLIC, _hello_oncreate_code>>

        align $04

        data_code:

        ; ── <init>()V ─────────────────────────────────────
        ; v0 = this (p0)
        dex_method _hello_init_code, $01, $01, $01
            invoke_direct  _activity_init_method, v0
            return_void
        end dex_method
        
        ; ── onCreate(Bundle)V ─────────────────────────────
        ; registers=4, ins=2: v2=this(p0), v3=bundle(p1)
        dex_method _hello_oncreate_code, $04, $02, $02
            invoke_super   _activity_oncreate_method, v2, v3
            new_instance   v0, _textview_type
            invoke_direct  _textview_init_method, v0, v2
            const_string   v1, _hello_world_string
            invoke_virtual _textview_settext_method, v0, v1
            invoke_virtual _activity_setcv_method, v2, v0
            return_void
        end dex_method

        dex_map_list

    dex_footer

end postpone
