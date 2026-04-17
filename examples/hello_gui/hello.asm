format binary as "dex"
org $00

include '../../forgedex.inc'

macro _strings
    defstr     _init,            "<init>"
    defstr     _hello_world,     "Hello, World!"
    defstrtype _activity,        "Landroid/app/Activity;"
    defstrtype _context,         "Landroid/content/Context;"
    defstrtype _bundle,          "Landroid/os/Bundle;"
    defstrtype _view,            "Landroid/view/View;"
    defstrtype _textview,        "Landroid/widget/TextView;"
    defstrtype _hello,           "Lapp/hello/HelloWorld;"
    defstrtype _charsequence,    "Ljava/lang/CharSequence;"
    defstrtype _object,          "Ljava/lang/Object;"
    defstrtype _V,               "V"
    defstr     _shorty_vl,       "VL"
    defstr     _oncreate,        "onCreate"
    defstr     _setcontentview,  "setContentView"
    defstr     _settext,         "setText"
end macro

postpone
__global::

    dex_header

    string_ids: emit_string_ids _strings
    type_ids:   emit_type_ids   _strings

    proto_ids:
        _void_v_proto     proto _V, _V, $00
        _context_v_proto  proto _shorty_vl, _V, _context_v_tl
        _bundle_v_proto   proto _shorty_vl, _V, _bundle_v_tl
        _view_v_proto     proto _shorty_vl, _V, _view_v_tl
        _charseq_v_proto  proto _shorty_vl, _V, _charseq_v_tl

    method_ids:
        _activity_init_method     method_id _activity_type, _void_v_proto, _init
        _activity_oncreate_method method_id _activity_type, _bundle_v_proto, _oncreate
        _activity_setcv_method    method_id _activity_type, _view_v_proto, _setcontentview
        _textview_init_method     method_id _textview_type, _context_v_proto, _init
        _textview_settext_method  method_id _textview_type, _charseq_v_proto, _settext
        _hello_init_method        method_id _hello_type,    _void_v_proto, _init
        _hello_oncreate_method    method_id _hello_type,    _bundle_v_proto, _oncreate

    class_defs:
        _hello_class public_class _hello_type, _activity_type, $00, _hello_class_data

    data:
        data_strings _strings


        data_type_lists
            _context_v_tl  type_list <_context>
            _bundle_v_tl   type_list <_bundle>
            _view_v_tl     type_list <_view>
            _charseq_v_tl  type_list <_charsequence>


        data_class_data
            _hello_class_data class_data_item \
                <>, <>, \
                <<_hello_init_method,     ACC_PUBLIC, _hello_init_code>>, \
                <<_hello_oncreate_method, ACC_PUBLIC, _hello_oncreate_code>>


        data_code

        ; ── <init>()V ─────────────────────────────────────
        ; p0 = this
        dex_method _hello_init_code, $01, $01, $01
            invoke_direct  _activity_init_method, p0
            return_void
        end dex_method
        
        ; ── onCreate(Bundle)V ─────────────────────────────
        ; registers=4, ins=2: p0=this, p1=bundle; v0, v1 are locals
        dex_method _hello_oncreate_code, $04, $02, $02
            invoke_super   _activity_oncreate_method, p0, p1
            new_instance   v0, _textview_type
            invoke_direct  _textview_init_method, v0, p0
            const_string   v1, _hello_world_string
            invoke_virtual _textview_settext_method, v0, v1
            invoke_virtual _activity_setcv_method, p0, v0
            return_void
        end dex_method

        dex_map_list

    dex_footer

end postpone
