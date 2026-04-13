format binary as "dex"
org $00

include '../../forgedex.inc'

postpone
__global::

    header:
        ubyte DEX_FILE_MAGIC
    header_magic_end:
        uint $00
    header_adler32:
        emit $14 : $00
    header_sha1:
        uint (header_end - header)          ; file_size
        uint (string_ids - header)          ; header_size
        uint ENDIAN_CONSTANT
        uint $00                            ; link_size
        uint $00                            ; link_off
        uint _map                           ; map_off
        uint string_ids_size
        uint (string_ids  - header)
        uint type_ids_size
        uint (type_ids    - header)
        uint proto_ids_size
        uint (proto_ids   - header)
        uint $00                            ; field_ids_size
        uint $00                            ; field_ids_off
        uint method_ids_size
        uint (method_ids  - header)
        uint class_defs_size
        uint (class_defs  - header)
        uint (link_data   - data)           ; data_size
        uint (data        - header)         ; data_off

    ; ── string_ids ────────────────────────────────────────
    ; Sorted by MUTF-8 unsigned byte order:
    ; '<'(3C) < 'H'(48) < 'L'(4C)
    ; "La..." < "Lj..." because 'a'(61) < 'j'(6A)
    ; Among "La...": "Landroid/..." < "Lapp/..." because 'n'(6E) < 'p'(70)
    string_ids:
        _init_string            string_id_item _init_data            ;  0 "<init>"
        _hello_world_string     string_id_item _hello_world_data     ;  1 "Hello, World!"
        _activity_string        string_id_item _activity_data        ;  2 "Landroid/app/Activity;"
        _context_string         string_id_item _context_data         ;  3 "Landroid/content/Context;"
        _bundle_string          string_id_item _bundle_data          ;  4 "Landroid/os/Bundle;"
        _view_string            string_id_item _view_data            ;  5 "Landroid/view/View;"
        _textview_string        string_id_item _textview_data        ;  6 "Landroid/widget/TextView;"
        _class_hello_string     string_id_item _class_hello_data     ;  7 "Lapp/hello/HelloWorld;"
        _charsequence_string    string_id_item _charsequence_data    ;  8 "Ljava/lang/CharSequence;"
        _object_string          string_id_item _object_data          ;  9 "Ljava/lang/Object;"
        _void_string            string_id_item _void_data            ; 10 "V"
        _shorty_vl_string       string_id_item _shorty_vl_data       ; 11 "VL"
        _oncreate_string        string_id_item _oncreate_data        ; 12 "onCreate"
        _setcontentview_string  string_id_item _setcontentview_data  ; 13 "setContentView"
        _settext_string         string_id_item _settext_data         ; 14 "setText"

    ; ── type_ids ──────────────────────────────────────────
    ; Sorted by string_idx ascending
    type_ids:
        _activity_type      type_id_item _activity_string       ; [0] string[2]
        _context_type       type_id_item _context_string        ; [1] string[3]
        _bundle_type        type_id_item _bundle_string         ; [2] string[4]
        _view_type          type_id_item _view_string           ; [3] string[5]
        _textview_type      type_id_item _textview_string       ; [4] string[6]
        _hello_type         type_id_item _class_hello_string    ; [5] string[7]
        _charsequence_type  type_id_item _charsequence_string   ; [6] string[8]
        _object_type        type_id_item _object_string         ; [7] string[9]
        _void_type          type_id_item _void_string           ; [8] string[10]

    ; ── proto_ids ─────────────────────────────────────────
    ; All return void (type[8]), sorted by param type_idx: none < 1 < 2 < 3 < 6
    proto_ids:
        _void_v_proto     proto_id_item _void_string,      _void_type, $00             ; [0] ()V
        _context_v_proto  proto_id_item _shorty_vl_string, _void_type, _context_v_tl  ; [1] (Context)V
        _bundle_v_proto   proto_id_item _shorty_vl_string, _void_type, _bundle_v_tl   ; [2] (Bundle)V
        _view_v_proto     proto_id_item _shorty_vl_string, _void_type, _view_v_tl     ; [3] (View)V
        _charseq_v_proto  proto_id_item _shorty_vl_string, _void_type, _charseq_v_tl  ; [4] (CharSequence)V

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
            _init_data           defstring "<init>"
            _hello_world_data    defstring "Hello, World!"
            _activity_data       defstring "Landroid/app/Activity;"
            _context_data        defstring "Landroid/content/Context;"
            _bundle_data         defstring "Landroid/os/Bundle;"
            _view_data           defstring "Landroid/view/View;"
            _textview_data       defstring "Landroid/widget/TextView;"
            _class_hello_data    defstring "Lapp/hello/HelloWorld;"
            _charsequence_data   defstring "Ljava/lang/CharSequence;"
            _object_data         defstring "Ljava/lang/Object;"
            _void_data           defstring "V"
            _shorty_vl_data      defstring "VL"
            _oncreate_data       defstring "onCreate"
            _setcontentview_data defstring "setContentView"
            _settext_data        defstring "setText"

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

    link_data:
    header_end:

    emit_checksums

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
