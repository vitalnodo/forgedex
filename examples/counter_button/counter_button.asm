; ══════════════════════════════════════════════════════════
;  Counter Activity
;
;  public class HelloWorld extends Activity
;                          implements View.OnClickListener {
;      int count = 0;
;      Button btn;
;
;      public void onCreate(Bundle b) {
;          super.onCreate(b);
;          btn = new Button(this);
;          btn.setText("0");
;          btn.setOnClickListener(this);
;          setContentView(btn);
;      }
;
;      public void onClick(View v) {
;          count++;
;          btn.setText(Integer.toString(count));
;      }
;  }
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _strings
    defstr _s0,       "0"
    defstr _init,     "<init>"
    defstr _I,        "I"
    defstr _LI,       "LI"
    defstr _activity, "Landroid/app/Activity;"
    defstr _context,  "Landroid/content/Context;"
    defstr _bundle,   "Landroid/os/Bundle;"
    defstr _oncl,     "Landroid/view/View$OnClickListener;"
    defstr _view,     "Landroid/view/View;"
    defstr _button,   "Landroid/widget/Button;"
    defstr _hello,    "Lapp/hello/counter_button/CounterButton;"
    defstr _charseq,  "Ljava/lang/CharSequence;"
    defstr _integer,  "Ljava/lang/Integer;"
    defstr _object,   "Ljava/lang/Object;"
    defstr _string,   "Ljava/lang/String;"
    defstr _V,        "V"
    defstr _VL,       "VL"
    defstr _btn,      "btn"
    defstr _count,    "count"
    defstr _onclick,  "onClick"
    defstr _oncreate, "onCreate"
    defstr _setcv,    "setContentView"
    defstr _setoncl,  "setOnClickListener"
    defstr _settext,  "setText"
    defstr _tostring, "toString"
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
        deftype _hello
        deftype _charseq
        deftype _integer
        deftype _object
        deftype _string
        deftype _V

    ; ── proto_ids ─────────────────────────────────────────
    ; Sorted by (return_type_idx, param_type_idx):
    ;   return String(11) before return void(12)
    ;   void protos: ()V, (Context)V, (Bundle)V, (OnCl)V, (View)V, (CharSeq)V
    proto_ids:
        _int_str_proto  proto_id_item _LI_string, _string_type, _int_tl      ; [0] (int)String
        _void_proto     proto_id_item _V_string,  _V_type,   $00          ; [1] ()V
        _context_proto  proto_id_item _VL_string, _V_type,   _context_tl  ; [2] (Context)V
        _bundle_proto   proto_id_item _VL_string, _V_type,   _bundle_tl   ; [3] (Bundle)V
        _oncl_proto     proto_id_item _VL_string, _V_type,   _oncl_tl     ; [4] (OnClickListener)V
        _view_proto     proto_id_item _VL_string, _V_type,   _view_tl     ; [5] (View)V
        _charseq_proto  proto_id_item _VL_string, _V_type,   _charseq_tl  ; [6] (CharSequence)V

    ; ── field_ids ─────────────────────────────────────────
    ; Sorted by (class_idx=7, type_idx): Button(6) < I(0) — wait: btn field_idx=0 < count field_idx=1
    field_ids:
        _btn_field   field_id_item _hello_type, _button_type, _btn_string   ; [0] HelloWorld.btn:Button
        _count_field field_id_item _hello_type, _I_type,      _count_string ; [1] HelloWorld.count:int

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _act_init_m   method_id_item _activity_type, _void_proto,    _init_string     ; [0] Activity.<init>
        _act_oncr_m   method_id_item _activity_type, _bundle_proto,  _oncreate_string ; [1] Activity.onCreate
        _act_setcv_m  method_id_item _activity_type, _view_proto,    _setcv_string    ; [2] Activity.setContentView
        _btn_init_m   method_id_item _button_type,   _context_proto, _init_string     ; [3] Button.<init>
        _btn_setocl_m method_id_item _button_type,   _oncl_proto,    _setoncl_string  ; [4] Button.setOnClickListener
        _btn_settt_m  method_id_item _button_type,   _charseq_proto, _settext_string  ; [5] Button.setText
        _hw_init_m    method_id_item _hello_type,    _void_proto,    _init_string     ; [6] HelloWorld.<init>
        _hw_ocl_m     method_id_item _hello_type,    _view_proto,    _onclick_string  ; [7] HelloWorld.onClick
        _hw_oncr_m    method_id_item _hello_type,    _bundle_proto,  _oncreate_string ; [8] HelloWorld.onCreate
        _int_tos_m    method_id_item _integer_type,  _int_str_proto, _tostring_string ; [9] Integer.toString

    ; ── class_defs ────────────────────────────────────────
    class_defs:
        _hello_class class_def_item \
            _hello_type, ACC_PUBLIC, _activity_type, \
            _hello_interfaces, NO_INDEX, $00, _hello_class_data, $00

    ; ══════════════════════════════════════════════════════
    ;  Data section
    ; ══════════════════════════════════════════════════════
    data:
        data_strings:
            emit_string_data _strings

        align $04

        data_type_lists:
            _int_tl      type_list $01, <_I_type>        ; param for proto[0]: (int)
            _context_tl  type_list $01, <_context_type>  ; param for proto[2]
            _bundle_tl   type_list $01, <_bundle_type>   ; param for proto[3]
            ; HelloWorld interfaces + proto[4] param share the same content [OnClickListener]
            _oncl_tl     type_list $01, <_oncl_type>
            _view_tl     type_list $01, <_view_type>
            _charseq_tl  type_list $01, <_charseq_type>

        ; interfaces_off for HelloWorld — list of interfaces implemented by the class
        align $04
        _hello_interfaces type_list $01, <_oncl_type>

        align $04

        ; static=0, instance=2(btn,count), direct=1(<init>), virtual=2(onClick,onCreate)
        data_class_data:
            _hello_class_data class_data_item $00, $02, $01, $02, \
                <>, \
                <<_btn_field,   ACC_PUBLIC>, <_count_field, ACC_PUBLIC>>, \
                <<_hw_init_m,  ACC_PUBLIC, _hw_init_code>>, \
                <<_hw_ocl_m,   ACC_PUBLIC, _hw_onclick_code>, \
                 <$01,         ACC_PUBLIC, _hw_oncreate_code>>

        align $04

        data_code:
        ; ── <init>()V ─────────────────────────────────────────────
        ; v0 = this (p0)
        virtual at $00
        _init_insns::
            invoke_direct  _act_init_m, v0                      ; Activity.<init>
            return_void
        end virtual

        ; <init>()V: registers=1, ins=1(v0=this), outs=1, 4 code units
        _hw_init_code    code_item $01, $01, $01, $00, $00, $04, _init_insns

        align $04

        ; ── onCreate(Bundle)V ─────────────────────────────────────
        ; registers=4, ins=2: v2=this(p0), v3=bundle(p1)
        ; v0=Button, v1="0"
        virtual at $00
        _oncreate_insns::
            invoke_super   _act_oncr_m, v2, v3                  ; Activity.onCreate
            new_instance   v0, _button_type                     ; new Button
            invoke_direct  _btn_init_m, v0, v2                  ; Button.<init>
            const_string   v1, _s0_string                          ; "0"
            invoke_virtual _btn_settt_m, v0, v1                 ; setText
            invoke_virtual _btn_setocl_m, v0, v2                ; setOnClickListener
            iput_object    v0, v2, _btn_field                   ; this.btn = btn
            invoke_virtual _act_setcv_m, v2, v0                 ; setContentView
            return_void
        end virtual

        ; onCreate(Bundle)V: registers=4, ins=2(v2=this, v3=bundle), outs=2, 22 code units
        _hw_oncreate_code code_item $04, $02, $02, $00, $00, $16, _oncreate_insns

        align $04

        ; ── onClick(View)V ────────────────────────────────────────
        ; registers=4, ins=2: v2=this(p0), v3=view(p1)
        ; v0=count(int)/result(String), v1=btn(Button)
        virtual at $00
        _onclick_insns::
            iget           v0, v2, _count_field                 ; count
            add_int_lit8   v0, v0, 1                            ; count++
            iput           v0, v2, _count_field                 ; this.count = count
            iget_object    v1, v2, _btn_field                   ; btn
            invoke_static  _int_tos_m, v0                       ; Integer.toString
            move_result_object v0                               ; result
            invoke_virtual _btn_settt_m, v1, v0                 ; setText
            return_void
        end virtual

        ; onClick(View)V: registers=4, ins=2(v2=this, v3=view), outs=2, 16 code units
        _hw_onclick_code  code_item $04, $02, $02, $00, $00, $10, _onclick_insns

        align $04

        dex_map_list

    dex_footer

end postpone
