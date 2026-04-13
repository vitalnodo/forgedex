; ══════════════════════════════════════════════════════════
;  Counter2 — TextView + Button(+) + Button(-) in LinearLayout
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _strings
    defstr _plus,      "+"
    defstr _minus,     "-"
    defstr _s0,        "0"
    defstr _init,      "<init>"
    defstr _I,         "I"
    defstr _LI,        "LI"
    defstr _activity,  "Landroid/app/Activity;"
    defstr _context,   "Landroid/content/Context;"
    defstr _bundle,    "Landroid/os/Bundle;"
    defstr _oncl,      "Landroid/view/View$OnClickListener;"
    defstr _view,      "Landroid/view/View;"
    defstr _button,    "Landroid/widget/Button;"
    defstr _ll,        "Landroid/widget/LinearLayout;"
    defstr _textview,  "Landroid/widget/TextView;"
    defstr _hello,     "Lapp/hello/counter_plus_minus/HelloWorld;"
    defstr _charseq,   "Ljava/lang/CharSequence;"
    defstr _integer,   "Ljava/lang/Integer;"
    defstr _object,    "Ljava/lang/Object;"
    defstr _string,    "Ljava/lang/String;"
    defstr _V,         "V"
    defstr _VI,        "VI"
    defstr _VL,        "VL"
    defstr _addview,   "addView"
    defstr _btn_minus, "btn_minus"
    defstr _btn_plus,  "btn_plus"
    defstr _count,     "count"
    defstr _onclick,   "onClick"
    defstr _oncreate,  "onCreate"
    defstr _setcv,     "setContentView"
    defstr _setoncl,   "setOnClickListener"
    defstr _setorient, "setOrientation"
    defstr _settext,   "setText"
    defstr _tostring,  "toString"
    defstr _tv,        "tv"
end macro

postpone
__global::

    dex_header

    string_ids:
        emit_string_ids _strings

    type_ids:
        deftype _I
        deftype _activity
        deftype _context
        deftype _bundle
        deftype _oncl
        deftype _view
        deftype _button
        deftype _ll
        deftype _textview
        deftype _hello
        deftype _charseq
        deftype _integer
        deftype _object
        deftype _string
        deftype _V


    ; ── proto_ids ─────────────────────────────────────────
    ; Sorted by (return_type_idx, first_param_type_idx):
    ;   return String[13] before return void[14]
    ;   void protos: ()V < (I)V[0] < (Context)V[2] < (Bundle)V[3] < (OnCl)V[4] < (View)V[5] < (CharSeq)V[10]
    proto_ids:
        _I_str_proto   proto_id_item _LI_string, _string_type, _I_tl        ; [0] (I)String
        _void_proto    proto_id_item _V_string,  _V_type,   $00          ; [1] ()V
        _I_v_proto     proto_id_item _VI_string, _V_type,   _I_tl        ; [2] (I)V
        _context_proto proto_id_item _VL_string, _V_type,   _context_tl  ; [3] (Context)V
        _bundle_proto  proto_id_item _VL_string, _V_type,   _bundle_tl   ; [4] (Bundle)V
        _oncl_proto    proto_id_item _VL_string, _V_type,   _oncl_tl     ; [5] (OnClickListener)V
        _view_proto    proto_id_item _VL_string, _V_type,   _view_tl     ; [6] (View)V
        _charseq_proto proto_id_item _VL_string, _V_type,   _charseq_tl  ; [7] (CharSequence)V

    ; ── field_ids ─────────────────────────────────────────
    ; Sorted by (class_idx=9, name_idx): btn_minus[23] < btn_plus[24] < count[25] < tv[33]
    field_ids:
        _f_btn_minus field_id_item _hello_type, _button_type,   _btn_minus_string ; [0]
        _f_btn_plus  field_id_item _hello_type, _button_type,   _btn_plus_string  ; [1]
        _f_count     field_id_item _hello_type, _I_type,        _count_string     ; [2]
        _f_tv        field_id_item _hello_type, _textview_type, _tv_string        ; [3]

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _act_init_m   method_id_item _activity_type, _void_proto,    _init_string     ; [0]
        _act_oncr_m   method_id_item _activity_type, _bundle_proto,  _oncreate_string ; [1]
        _act_setcv_m  method_id_item _activity_type, _view_proto,    _setcv_string    ; [2]
        _btn_init_m   method_id_item _button_type,   _context_proto, _init_string     ; [3]
        _btn_setocl_m method_id_item _button_type,   _oncl_proto,    _setoncl_string  ; [4]
        _btn_settt_m  method_id_item _button_type,   _charseq_proto, _settext_string  ; [5]
        _ll_init_m    method_id_item _ll_type,       _context_proto, _init_string     ; [6]
        _ll_addview_m method_id_item _ll_type,       _view_proto,    _addview_string  ; [7]
        _ll_setor_m   method_id_item _ll_type,       _I_v_proto,     _setorient_string; [8]
        _tv_init_m    method_id_item _textview_type, _context_proto, _init_string     ; [9]
        _tv_settt_m   method_id_item _textview_type, _charseq_proto, _settext_string  ; [10]
        _hw_init_m    method_id_item _hello_type,    _void_proto,    _init_string     ; [11]
        _hw_ocl_m     method_id_item _hello_type,    _view_proto,    _onclick_string  ; [12]
        _hw_oncr_m    method_id_item _hello_type,    _bundle_proto,  _oncreate_string ; [13]
        _int_tos_m    method_id_item _integer_type,  _I_str_proto,   _tostring_string ; [14]

    class_defs:
        _hello_class class_def_item \
            _hello_type, ACC_PUBLIC, _activity_type, \
            _hello_ifaces, NO_INDEX, $00, _hello_class_data, $00

    ; ══════════════════════════════════════════════════════
    ;  Data section
    ; ══════════════════════════════════════════════════════
    data:
        data_strings:
            emit_string_data _strings

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

    dex_footer

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
    const_string   v4, _s0_string                          ; "0"
    invoke_virtual _tv_settt_m, v1, v4                  ; setText
    iput_object    v1, v5, _f_tv                        ; this.tv = tv
    new_instance   v2, _button_type                     ; new Button
    invoke_direct  _btn_init_m, v2, v5                  ; Button.<init>
    const_string   v4, _plus_string                        ; "+"
    invoke_virtual _btn_settt_m, v2, v4                 ; setText
    invoke_virtual _btn_setocl_m, v2, v5                ; setOnClickListener
    iput_object    v2, v5, _f_btn_plus                  ; this.btn_plus = btn
    new_instance   v3, _button_type                     ; new Button
    invoke_direct  _btn_init_m, v3, v5                  ; Button.<init>
    const_string   v4, _minus_string                       ; "-"
    invoke_virtual _btn_settt_m, v3, v4                 ; setText
    invoke_virtual _btn_setocl_m, v3, v5                ; setOnClickListener
    iput_object    v3, v5, _f_btn_minus                 ; this.btn_minus = btn
    invoke_virtual _ll_addview_m, v0, v1                ; addView(tv)
    invoke_virtual _ll_addview_m, v0, v3                ; addView(btn_minus)
    invoke_virtual _ll_addview_m, v0, v2                ; addView(btn_plus)
    invoke_virtual _act_setcv_m, v5, v0                 ; setContentView
    return_void
end virtual
