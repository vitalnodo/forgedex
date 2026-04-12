; ══════════════════════════════════════════════════════════
;  Counter2 — TextView + Button(+) + Button(-) in LinearLayout
; ══════════════════════════════════════════════════════════

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
        uint field_ids_size
        uint (field_ids   - header)
        uint method_ids_size
        uint (method_ids  - header)
        uint class_defs_size
        uint (class_defs  - header)
        uint (link_data   - data)           ; data_size
        uint (data        - header)         ; data_off

    ; ── string_ids ────────────────────────────────────────
    ; Sorted by MUTF-8 unsigned byte order
    string_ids:
        _plus_str        string_id_item _plus_data        ;  0 "+"
        _minus_str       string_id_item _minus_data       ;  1 "-"
        _s0_str          string_id_item _s0_data          ;  2 "0"
        _init_str        string_id_item _init_data        ;  3 "<init>"
        _I_str           string_id_item _I_data           ;  4 "I"
        _LI_str          string_id_item _LI_data          ;  5 "LI"
        _activity_str    string_id_item _activity_data    ;  6 "Landroid/app/Activity;"
        _context_str     string_id_item _context_data     ;  7 "Landroid/content/Context;"
        _bundle_str      string_id_item _bundle_data      ;  8 "Landroid/os/Bundle;"
        _oncl_str        string_id_item _oncl_data        ;  9 "Landroid/view/View$OnClickListener;"
        _view_str        string_id_item _view_data        ; 10 "Landroid/view/View;"
        _button_str      string_id_item _button_data      ; 11 "Landroid/widget/Button;"
        _ll_str          string_id_item _ll_data          ; 12 "Landroid/widget/LinearLayout;"
        _textview_str    string_id_item _textview_data    ; 13 "Landroid/widget/TextView;"
        _hello_str       string_id_item _hello_data       ; 14 "Lapp/hello/counter_plus_minus/HelloWorld;"
        _charseq_str     string_id_item _charseq_data     ; 15 "Ljava/lang/CharSequence;"
        _integer_str     string_id_item _integer_data     ; 16 "Ljava/lang/Integer;"
        _object_str      string_id_item _object_data      ; 17 "Ljava/lang/Object;"
        _string_str      string_id_item _string_data      ; 18 "Ljava/lang/String;"
        _V_str           string_id_item _V_data           ; 19 "V"
        _VI_str          string_id_item _VI_data          ; 20 "VI"
        _VL_str          string_id_item _VL_data          ; 21 "VL"
        _addview_str     string_id_item _addview_data     ; 22 "addView"
        _btn_minus_str   string_id_item _btn_minus_data   ; 23 "btn_minus"
        _btn_plus_str    string_id_item _btn_plus_data    ; 24 "btn_plus"
        _count_str       string_id_item _count_data       ; 25 "count"
        _onclick_str     string_id_item _onclick_data     ; 26 "onClick"
        _oncreate_str    string_id_item _oncreate_data    ; 27 "onCreate"
        _setcv_str       string_id_item _setcv_data       ; 28 "setContentView"
        _setoncl_str     string_id_item _setoncl_data     ; 29 "setOnClickListener"
        _setorient_str   string_id_item _setorient_data   ; 30 "setOrientation"
        _settext_str     string_id_item _settext_data     ; 31 "setText"
        _tostring_str    string_id_item _tostring_data    ; 32 "toString"
        _tv_str          string_id_item _tv_data          ; 33 "tv"

    ; ── type_ids ──────────────────────────────────────────
    ; Sorted by string_idx ascending
    type_ids:
        _I_type        type_id_item _I_str          ; [0]  int
        _activity_type type_id_item _activity_str   ; [1]  Activity
        _context_type  type_id_item _context_str    ; [2]  Context
        _bundle_type   type_id_item _bundle_str     ; [3]  Bundle
        _oncl_type     type_id_item _oncl_str       ; [4]  OnClickListener
        _view_type     type_id_item _view_str       ; [5]  View
        _button_type   type_id_item _button_str     ; [6]  Button
        _ll_type       type_id_item _ll_str         ; [7]  LinearLayout
        _textview_type type_id_item _textview_str   ; [8]  TextView
        _hello_type    type_id_item _hello_str      ; [9]  HelloWorld
        _charseq_type  type_id_item _charseq_str    ; [10] CharSequence
        _integer_type  type_id_item _integer_str    ; [11] Integer
        _object_type   type_id_item _object_str     ; [12] Object
        _string_type   type_id_item _string_str     ; [13] String
        _void_type     type_id_item _V_str          ; [14] V

    ; ── proto_ids ─────────────────────────────────────────
    ; Sorted by (return_type_idx, first_param_type_idx):
    ;   return String[13] before return void[14]
    ;   void protos: ()V < (I)V[0] < (Context)V[2] < (Bundle)V[3] < (OnCl)V[4] < (View)V[5] < (CharSeq)V[10]
    proto_ids:
        _I_str_proto   proto_id_item _LI_str, _string_type, _I_tl        ; [0] (I)String
        _void_proto    proto_id_item _V_str,  _void_type,   $00          ; [1] ()V
        _I_v_proto     proto_id_item _VI_str, _void_type,   _I_tl        ; [2] (I)V
        _context_proto proto_id_item _VL_str, _void_type,   _context_tl  ; [3] (Context)V
        _bundle_proto  proto_id_item _VL_str, _void_type,   _bundle_tl   ; [4] (Bundle)V
        _oncl_proto    proto_id_item _VL_str, _void_type,   _oncl_tl     ; [5] (OnClickListener)V
        _view_proto    proto_id_item _VL_str, _void_type,   _view_tl     ; [6] (View)V
        _charseq_proto proto_id_item _VL_str, _void_type,   _charseq_tl  ; [7] (CharSequence)V

    ; ── field_ids ─────────────────────────────────────────
    ; Sorted by (class_idx=9, name_idx): btn_minus[23] < btn_plus[24] < count[25] < tv[33]
    field_ids:
        _f_btn_minus field_id_item _hello_type, _button_type,   _btn_minus_str ; [0]
        _f_btn_plus  field_id_item _hello_type, _button_type,   _btn_plus_str  ; [1]
        _f_count     field_id_item _hello_type, _I_type,        _count_str     ; [2]
        _f_tv        field_id_item _hello_type, _textview_type, _tv_str        ; [3]

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _act_init_m   method_id_item _activity_type, _void_proto,    _init_str     ; [0]
        _act_oncr_m   method_id_item _activity_type, _bundle_proto,  _oncreate_str ; [1]
        _act_setcv_m  method_id_item _activity_type, _view_proto,    _setcv_str    ; [2]
        _btn_init_m   method_id_item _button_type,   _context_proto, _init_str     ; [3]
        _btn_setocl_m method_id_item _button_type,   _oncl_proto,    _setoncl_str  ; [4]
        _btn_settt_m  method_id_item _button_type,   _charseq_proto, _settext_str  ; [5]
        _ll_init_m    method_id_item _ll_type,       _context_proto, _init_str     ; [6]
        _ll_addview_m method_id_item _ll_type,       _view_proto,    _addview_str  ; [7]
        _ll_setor_m   method_id_item _ll_type,       _I_v_proto,     _setorient_str; [8]
        _tv_init_m    method_id_item _textview_type, _context_proto, _init_str     ; [9]
        _tv_settt_m   method_id_item _textview_type, _charseq_proto, _settext_str  ; [10]
        _hw_init_m    method_id_item _hello_type,    _void_proto,    _init_str     ; [11]
        _hw_ocl_m     method_id_item _hello_type,    _view_proto,    _onclick_str  ; [12]
        _hw_oncr_m    method_id_item _hello_type,    _bundle_proto,  _oncreate_str ; [13]
        _int_tos_m    method_id_item _integer_type,  _I_str_proto,   _tostring_str ; [14]

    class_defs:
        _hello_class class_def_item \
            _hello_type, ACC_PUBLIC, _activity_type, \
            _hello_ifaces, NO_INDEX, $00, _hello_class_data, $00

    ; ══════════════════════════════════════════════════════
    ;  Data section
    ; ══════════════════════════════════════════════════════
    data:

        data_strings:
            _plus_data      string_data_item $01, "+"
            _minus_data     string_data_item $01, "-"
            _s0_data        string_data_item $01, "0"
            _init_data      string_data_item $06, "<init>"
            _I_data         string_data_item $01, "I"
            _LI_data        string_data_item $02, "LI"
            _activity_data  string_data_item $16, "Landroid/app/Activity;"
            _context_data   string_data_item $19, "Landroid/content/Context;"
            _bundle_data    string_data_item $13, "Landroid/os/Bundle;"
            _oncl_data      string_data_item $23, "Landroid/view/View$OnClickListener;"
            _view_data      string_data_item $13, "Landroid/view/View;"
            _button_data    string_data_item $17, "Landroid/widget/Button;"
            _ll_data        string_data_item $1D, "Landroid/widget/LinearLayout;"
            _textview_data  string_data_item $19, "Landroid/widget/TextView;"
            _hello_data     string_data_item $29, "Lapp/hello/counter_plus_minus/HelloWorld;"
            _charseq_data   string_data_item $18, "Ljava/lang/CharSequence;"
            _integer_data   string_data_item $13, "Ljava/lang/Integer;"
            _object_data    string_data_item $12, "Ljava/lang/Object;"
            _string_data    string_data_item $12, "Ljava/lang/String;"
            _V_data         string_data_item $01, "V"
            _VI_data        string_data_item $02, "VI"
            _VL_data        string_data_item $02, "VL"
            _addview_data   string_data_item $07, "addView"
            _btn_minus_data string_data_item $09, "btn_minus"
            _btn_plus_data  string_data_item $08, "btn_plus"
            _count_data     string_data_item $05, "count"
            _onclick_data   string_data_item $07, "onClick"
            _oncreate_data  string_data_item $08, "onCreate"
            _setcv_data     string_data_item $0E, "setContentView"
            _setoncl_data   string_data_item $12, "setOnClickListener"
            _setorient_data string_data_item $0E, "setOrientation"
            _settext_data   string_data_item $07, "setText"
            _tostring_data  string_data_item $08, "toString"
            _tv_data        string_data_item $02, "tv"

        align $04

        data_type_lists:
            _I_tl       type_list $01, <_I_type>        ; param for proto[0,2]: (I)
            _context_tl type_list $01, <_context_type>  ; param for proto[3]
            _bundle_tl  type_list $01, <_bundle_type>   ; param for proto[4]
            _oncl_tl    type_list $01, <_oncl_type>     ; param for proto[5]
            _view_tl    type_list $01, <_view_type>     ; param for proto[6]
            _charseq_tl type_list $01, <_charseq_type>  ; param for proto[7]

        ; HelloWorld implements OnClickListener — reuse _oncl_tl (same content, same offset)
        _hello_ifaces = _oncl_tl

        align $04

        ; static=0, instance=4(btn_minus,btn_plus,count,tv), direct=1(<init>), virtual=2(onClick,onCreate)
        data_class_data:
            _hello_class_data class_data_item $00, $04, $01, $02, \
                <>, \
                <<_f_btn_minus, ACC_PUBLIC>, \
                 <$01, ACC_PUBLIC>, \
                 <$01, ACC_PUBLIC>, \
                 <$01, ACC_PUBLIC>>, \
                <<_hw_init_m, ACC_PUBLIC, _hw_init_code>>, \
                <<_hw_ocl_m,  ACC_PUBLIC, _hw_onclick_code>, \
                 <$01,        ACC_PUBLIC, _hw_oncreate_code>>

        align $04

        ; <init>()V: registers=1, ins=1(v0=this), outs=1, 4 code units
        _hw_init_code    code_item $01, $01, $01, $00, $00, $04, _init_insns

        align $04

        ; onClick(View)V: registers=6, ins=2(v4=this, v5=view), outs=2, 27 code units
        _hw_onclick_code code_item $06, $02, $02, $00, $00, $1B, _onclick_insns

        align $04

        ; onCreate(Bundle)V: registers=7, ins=2(v5=this, v6=bundle), outs=2, 67 code units
        _hw_oncreate_code code_item $07, $02, $02, $00, $00, $43, _oncreate_insns

        align $04

        data_map:
            _map map_list $0C, <\
                <TYPE_HEADER_ITEM,      $01,             header>,\
                <TYPE_STRING_ID_ITEM,   string_ids_size, string_ids>,\
                <TYPE_TYPE_ID_ITEM,     type_ids_size,   type_ids>,\
                <TYPE_PROTO_ID_ITEM,    proto_ids_size,  proto_ids>,\
                <TYPE_FIELD_ID_ITEM,    field_ids_size,  field_ids>,\
                <TYPE_METHOD_ID_ITEM,   method_ids_size, method_ids>,\
                <TYPE_CLASS_DEF_ITEM,   class_defs_size, class_defs>,\
                <TYPE_STRING_DATA_ITEM, $22,             data_strings>,\
                <TYPE_TYPE_LIST,        $06,             data_type_lists>,\
                <TYPE_CLASS_DATA_ITEM,  $01,             data_class_data>,\
                <TYPE_CODE_ITEM,        $03,             _hw_init_code>,\
                <TYPE_MAP_LIST,         $01,             data_map>\
            >

        align $04

    link_data:
    header_end:

    __snapshot_index string_ids_size, __string_id_item
    __snapshot_index type_ids_size,   __type_id_item
    __snapshot_index proto_ids_size,  __proto_id_item
    __snapshot_index field_ids_size,  __field_id_item
    __snapshot_index method_ids_size, __method_id_item
    __snapshot_index class_defs_size, __class_def_item

    load sha1_payload : ($ - header_sha1) from __global : header_sha1
    sha1_result sha1 sha1_payload
    store sha1_result : $14 at __global : header_adler32

    load adler32_payload : ($ - header_adler32) from __global : header_adler32
    adler32_result adler32 adler32_payload
    store adler32_result : $04 at __global : header_magic_end

end postpone

; ── <init>()V ─────────────────────────────────────────────
; v0=this(p0)
virtual at $00
_init_insns::
    invoke_direct  _act_init_m, v0                      ; Activity.<init>
    return_void
end virtual

; ── onClick(View)V ────────────────────────────────────────
; registers=6, ins=2: v4=this(p0), v5=view(p1)
; v0=count/str, v1=btn_plus ref, v2=tv ref
;
; if view == btn_plus → increment, else decrement (skip if already 0)
virtual at $00
_onclick_insns::
    iget_object    v1, v4, _f_btn_plus                  ; btn_plus
    if_eq          v5, v1, $0009                        ; if view == btn_plus → :plus
    iget           v0, v4, _f_count                     ; minus: count
    if_eqz         v0, $0009                            ; if count == 0 → :upd
    add_int_lit8   v0, v0, -1                           ; count--
    goto           $05                                  ; → :upd
    iget           v0, v4, _f_count                     ; :plus count
    add_int_lit8   v0, v0, 1                            ; count++
    iput           v0, v4, _f_count                     ; :upd this.count = count
    iget_object    v2, v4, _f_tv                        ; tv
    invoke_static  _int_tos_m, v0                       ; Integer.toString
    move_result_object v0                               ; result
    invoke_virtual _tv_settt_m, v2, v0                  ; setText
    return_void
end virtual

; ── onCreate(Bundle)V ─────────────────────────────────────
; registers=7, ins=2: v5=this(p0), v6=bundle(p1)
; v0=LinearLayout, v1=TextView, v2=Button(+), v3=Button(-), v4=temp
virtual at $00
_oncreate_insns::
    invoke_super   _act_oncr_m, v5, v6                  ; Activity.onCreate
    new_instance   v0, _ll_type                         ; new LinearLayout
    invoke_direct  _ll_init_m, v0, v5                   ; LinearLayout.<init>
    const_4        v4, 1                                ; VERTICAL
    invoke_virtual _ll_setor_m, v0, v4                  ; setOrientation
    new_instance   v1, _textview_type                   ; new TextView
    invoke_direct  _tv_init_m, v1, v5                   ; TextView.<init>
    const_string   v4, _s0_str                          ; "0"
    invoke_virtual _tv_settt_m, v1, v4                  ; setText
    iput_object    v1, v5, _f_tv                        ; this.tv = tv
    new_instance   v2, _button_type                     ; new Button
    invoke_direct  _btn_init_m, v2, v5                  ; Button.<init>
    const_string   v4, _plus_str                        ; "+"
    invoke_virtual _btn_settt_m, v2, v4                 ; setText
    invoke_virtual _btn_setocl_m, v2, v5                ; setOnClickListener
    iput_object    v2, v5, _f_btn_plus                  ; this.btn_plus = btn
    new_instance   v3, _button_type                     ; new Button
    invoke_direct  _btn_init_m, v3, v5                  ; Button.<init>
    const_string   v4, _minus_str                       ; "-"
    invoke_virtual _btn_settt_m, v3, v4                 ; setText
    invoke_virtual _btn_setocl_m, v3, v5                ; setOnClickListener
    iput_object    v3, v5, _f_btn_minus                 ; this.btn_minus = btn
    invoke_virtual _ll_addview_m, v0, v1                ; addView(tv)
    invoke_virtual _ll_addview_m, v0, v3                ; addView(btn_minus)
    invoke_virtual _ll_addview_m, v0, v2                ; addView(btn_plus)
    invoke_virtual _act_setcv_m, v5, v0                 ; setContentView
    return_void
end virtual
