; ══════════════════════════════════════════════════════════
;  Counter2 — TextView + Button(+) + Button(-) in LinearLayout
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _strings
    defstr     _plus,      "+"
    defstr     _minus,     "-"
    defstr     _s0,        "0"
    defstr     _init,      "<init>"
    defstrtype _I,         "I"
    defstr     _LI,        "LI"
    defstrtype _activity,  "Landroid/app/Activity;"
    defstrtype _context,   "Landroid/content/Context;"
    defstrtype _bundle,    "Landroid/os/Bundle;"
    defstrtype _oncl,      "Landroid/view/View$OnClickListener;"
    defstrtype _view,      "Landroid/view/View;"
    defstrtype _button,    "Landroid/widget/Button;"
    defstrtype _ll,        "Landroid/widget/LinearLayout;"
    defstrtype _textview,  "Landroid/widget/TextView;"
    defstrtype _hello,     "Lapp/hello/counter_plus_minus/HelloWorld;"
    defstrtype _charseq,   "Ljava/lang/CharSequence;"
    defstrtype _integer,   "Ljava/lang/Integer;"
    defstrtype _object,    "Ljava/lang/Object;"
    defstrtype _string,    "Ljava/lang/String;"
    defstrtype _V,         "V"
    defstr     _VI,        "VI"
    defstr     _VL,        "VL"
    defstr     _addview,   "addView"
    defstr     _btn_minus, "btn_minus"
    defstr     _btn_plus,  "btn_plus"
    defstr     _count,     "count"
    defstr     _onclick,   "onClick"
    defstr     _oncreate,  "onCreate"
    defstr     _setcv,     "setContentView"
    defstr     _setoncl,   "setOnClickListener"
    defstr     _setorient, "setOrientation"
    defstr     _settext,   "setText"
    defstr     _tostring,  "toString"
    defstr     _tv,        "tv"
end macro

postpone
__global::

    dex_header

    string_ids: emit_string_ids _strings
    type_ids:   emit_type_ids   _strings


    ; ── proto_ids ─────────────────────────────────────────
    ; Sorted by (return_type_idx, first_param_type_idx):
    ;   return String[13] before return void[14]
    ;   void protos: ()V < (I)V[0] < (Context)V[2] < (Bundle)V[3] < (OnCl)V[4] < (View)V[5] < (CharSeq)V[10]
    proto_ids:
        _I_str_proto   proto _LI, _string, _I_tl        ; [0] (I)String
        _void_proto    proto _V, _V,   $00          ; [1] ()V
        _I_v_proto     proto _VI, _V,   _I_tl        ; [2] (I)V
        _context_proto proto _VL, _V,   _context_tl  ; [3] (Context)V
        _bundle_proto  proto _VL, _V,   _bundle_tl   ; [4] (Bundle)V
        _oncl_proto    proto _VL, _V,   _oncl_tl     ; [5] (OnClickListener)V
        _view_proto    proto _VL, _V,   _view_tl     ; [6] (View)V
        _charseq_proto proto _VL, _V,   _charseq_tl  ; [7] (CharSequence)V

    ; ── field_ids ─────────────────────────────────────────
    ; Sorted by (class_idx, type_idx, name_idx): I(0) < Button(6) < TextView(8)
    field_ids:
        _f_count     field_id _hello_type, _I_type,        _count     ; [0]
        _f_btn_minus field_id _hello_type, _button_type,   _btn_minus ; [1]
        _f_btn_plus  field_id _hello_type, _button_type,   _btn_plus  ; [2]
        _f_tv        field_id _hello_type, _textview_type, _tv        ; [3]

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _act_init_m   method_id _activity_type, _void_proto, _init     ; [0]
        _act_oncr_m   method_id _activity_type, _bundle_proto, _oncreate ; [1]
        _act_setcv_m  method_id _activity_type, _view_proto, _setcv    ; [2]
        _btn_init_m   method_id _button_type,   _context_proto, _init     ; [3]
        _btn_setocl_m method_id _button_type,   _oncl_proto, _setoncl  ; [4]
        _btn_settt_m  method_id _button_type,   _charseq_proto, _settext  ; [5]
        _ll_setor_m   method_id _ll_type,       _I_v_proto,     _setorient ; [6]
        _ll_init_m    method_id _ll_type,       _context_proto, _init      ; [7]
        _ll_addview_m method_id _ll_type,       _view_proto,    _addview   ; [8]
        _tv_init_m    method_id _textview_type, _context_proto, _init     ; [9]
        _tv_settt_m   method_id _textview_type, _charseq_proto, _settext  ; [10]
        _hw_init_m    method_id _hello_type,    _void_proto,    _init     ; [11]
        _hw_oncr_m    method_id _hello_type,    _bundle_proto,  _oncreate ; [12]
        _hw_ocl_m     method_id _hello_type,    _view_proto,    _onclick  ; [13]
        _int_tos_m    method_id _integer_type,  _I_str_proto, _tostring ; [14]

    class_defs:
        _hello_class public_class _hello_type, _activity_type, _hello_ifaces, _hello_class_data

    ; ══════════════════════════════════════════════════════
    ;  Data section
    ; ══════════════════════════════════════════════════════
    data:
        data_strings _strings


        data_type_lists
            _I_tl       type_list <_I>        ; param for proto[0,2]: (I)
            _context_tl type_list <_context>  ; param for proto[3]
            _bundle_tl  type_list <_bundle>   ; param for proto[4]
            _oncl_tl    type_list <_oncl>     ; param for proto[5]
            _view_tl    type_list <_view>     ; param for proto[6]
            _charseq_tl type_list <_charseq>  ; param for proto[7]

        ; HelloWorld implements OnClickListener — reuse _oncl_tl (same content, same offset)
        _hello_ifaces = _oncl_tl


        ; static=0, instance=4(btn_minus,btn_plus,count,tv), direct=1(<init>), virtual=2(onClick,onCreate)
        data_class_data
            _hello_class_data class_data_item \
                <>, \
                <<_f_btn_minus, ACC_PUBLIC>, \
                 <_f_btn_plus,  ACC_PUBLIC>, \
                 <_f_count,     ACC_PUBLIC>, \
                 <_f_tv,        ACC_PUBLIC>>, \
                <<_hw_init_m, ACC_PUBLIC, _hw_init_code>>, \
                <<_hw_ocl_m,  ACC_PUBLIC, _hw_onclick_code>, \
                 <_hw_oncr_m, ACC_PUBLIC, _hw_oncreate_code>>


        data_code
        ; ── <init>()V ─────────────────────────────────────────────
        ; p0=this
        dex_method _hw_init_code, $01, $01, $01
            invoke_direct  _act_init_m, p0                      ; Activity.<init>
            return_void
        end dex_method

        ; ── onClick(View)V ────────────────────────────────────────
        ; registers=6, ins=2: p0=this, p1=view
        ; v0=count/str, v1=btn_plus ref, v2=tv ref
        ;
        ; if view == btn_plus → increment, else decrement (skip if already 0)
        dex_method _hw_onclick_code, $06, $02, $02
            iget_object    v1, p0, _f_btn_plus                  ; btn_plus
            if_eq          p1, v1, .plus                        ; if view == btn_plus → :plus
            iget           v0, p0, _f_count                     ; minus: count
            if_eqz         v0, .upd                             ; if count == 0 → :upd
            add_int_lit8   v0, v0, -1                           ; count--
            goto           .upd                                 ; → :upd
          .plus:
            iget           v0, p0, _f_count                     ; count
            add_int_lit8   v0, v0, 1                            ; count++
          .upd:
            iput           v0, p0, _f_count                     ; this.count = count
            iget_object    v2, p0, _f_tv                        ; tv
            invoke_static  _int_tos_m, v0                       ; Integer.toString
            move_result_object v0                               ; result
            invoke_virtual _tv_settt_m, v2, v0                  ; setText
            return_void
        end dex_method

        ; ── onCreate(Bundle)V ─────────────────────────────────────
        ; registers=7, ins=2: p0=this, p1=bundle
        ; v0=LinearLayout, v1=TextView, v2=Button(+), v3=Button(-), v4=temp
        dex_method _hw_oncreate_code, $07, $02, $02
            invoke_super   _act_oncr_m, p0, p1                  ; Activity.onCreate
            new_instance   v0, _ll_type                         ; new LinearLayout
            invoke_direct  _ll_init_m, v0, p0                   ; LinearLayout.<init>
            const_4        v4, 1                                ; VERTICAL
            invoke_virtual _ll_setor_m, v0, v4                  ; setOrientation
            new_instance   v1, _textview_type                   ; new TextView
            invoke_direct  _tv_init_m, v1, p0                   ; TextView.<init>
            const_string   v4, _s0_string                       ; "0"
            invoke_virtual _tv_settt_m, v1, v4                  ; setText
            iput_object    v1, p0, _f_tv                        ; this.tv = tv
            new_instance   v2, _button_type                     ; new Button
            invoke_direct  _btn_init_m, v2, p0                  ; Button.<init>
            const_string   v4, _plus_string                     ; "+"
            invoke_virtual _btn_settt_m, v2, v4                 ; setText
            invoke_virtual _btn_setocl_m, v2, p0                ; setOnClickListener
            iput_object    v2, p0, _f_btn_plus                  ; this.btn_plus = btn
            new_instance   v3, _button_type                     ; new Button
            invoke_direct  _btn_init_m, v3, p0                  ; Button.<init>
            const_string   v4, _minus_string                    ; "-"
            invoke_virtual _btn_settt_m, v3, v4                 ; setText
            invoke_virtual _btn_setocl_m, v3, p0                ; setOnClickListener
            iput_object    v3, p0, _f_btn_minus                 ; this.btn_minus = btn
            invoke_virtual _ll_addview_m, v0, v1                ; addView(tv)
            invoke_virtual _ll_addview_m, v0, v3                ; addView(btn_minus)
            invoke_virtual _ll_addview_m, v0, v2                ; addView(btn_plus)
            invoke_virtual _act_setcv_m, p0, v0                 ; setContentView
            return_void
        end dex_method

        dex_map_list

    dex_footer

end postpone

