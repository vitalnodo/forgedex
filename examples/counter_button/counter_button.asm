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
    defstr     _s0,       "0"
    defstr     _init,     "<init>"
    defstrtype _I,        "I"
    defstr     _LI,       "LI"
    defstrtype _activity, "Landroid/app/Activity;"
    defstrtype _context,  "Landroid/content/Context;"
    defstrtype _bundle,   "Landroid/os/Bundle;"
    defstrtype _oncl,     "Landroid/view/View$OnClickListener;"
    defstrtype _view,     "Landroid/view/View;"
    defstrtype _button,   "Landroid/widget/Button;"
    defstrtype _hello,    "Lapp/hello/counter_button/CounterButton;"
    defstrtype _charseq,  "Ljava/lang/CharSequence;"
    defstrtype _integer,  "Ljava/lang/Integer;"
    defstrtype _object,   "Ljava/lang/Object;"
    defstrtype _string,   "Ljava/lang/String;"
    defstrtype _V,        "V"
    defstr     _VL,       "VL"
    defstr     _btn,      "btn"
    defstr     _count,    "count"
    defstr     _onclick,  "onClick"
    defstr     _oncreate, "onCreate"
    defstr     _setcv,    "setContentView"
    defstr     _setoncl,  "setOnClickListener"
    defstr     _settext,  "setText"
    defstr     _tostring, "toString"
end macro

postpone
__global::

    dex_header

    string_ids: emit_string_ids _strings
    type_ids:   emit_type_ids   _strings

    ; ── proto_ids ─────────────────────────────────────────
    ; Sorted by (return_type_idx, param_type_idx):
    ;   return String(11) before return void(12)
    ;   void protos: ()V, (Context)V, (Bundle)V, (OnCl)V, (View)V, (CharSeq)V
    proto_ids:
        _int_str_proto  proto _LI, _string, _int_tl      ; [0] (int)String
        _void_proto     proto _V, _V,   $00          ; [1] ()V
        _context_proto  proto _VL, _V,   _context_tl  ; [2] (Context)V
        _bundle_proto   proto _VL, _V,   _bundle_tl   ; [3] (Bundle)V
        _oncl_proto     proto _VL, _V,   _oncl_tl     ; [4] (OnClickListener)V
        _view_proto     proto _VL, _V,   _view_tl     ; [5] (View)V
        _charseq_proto  proto _VL, _V,   _charseq_tl  ; [6] (CharSequence)V

    ; ── field_ids ─────────────────────────────────────────
    ; Sorted by (class_idx, type_idx, name_idx)
    field_ids:
        _count_field field_id _hello_type, _I_type,      _count ; [0] HelloWorld.count:int
        _btn_field   field_id _hello_type, _button_type, _btn   ; [1] HelloWorld.btn:Button

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _act_init_m   method_id _activity_type, _void_proto, _init     ; [0] Activity.<init>
        _act_oncr_m   method_id _activity_type, _bundle_proto, _oncreate ; [1] Activity.onCreate
        _act_setcv_m  method_id _activity_type, _view_proto, _setcv    ; [2] Activity.setContentView
        _btn_init_m   method_id _button_type,   _context_proto, _init     ; [3] Button.<init>
        _btn_setocl_m method_id _button_type,   _oncl_proto, _setoncl  ; [4] Button.setOnClickListener
        _btn_settt_m  method_id _button_type,   _charseq_proto, _settext  ; [5] Button.setText
        _hw_init_m    method_id _hello_type,    _void_proto,    _init     ; [6] HelloWorld.<init>
        _hw_oncr_m    method_id _hello_type,    _bundle_proto,  _oncreate ; [7] HelloWorld.onCreate
        _hw_ocl_m     method_id _hello_type,    _view_proto,    _onclick  ; [8] HelloWorld.onClick
        _int_tos_m    method_id _integer_type,  _int_str_proto, _tostring ; [9] Integer.toString

    ; ── class_defs ────────────────────────────────────────
    class_defs:
        _hello_class public_class _hello_type, _activity_type, _hello_interfaces, _hello_class_data

    ; ══════════════════════════════════════════════════════
    ;  Data section
    ; ══════════════════════════════════════════════════════
    data:
        data_strings _strings


        data_type_lists
            _int_tl      type_list <_I>        ; param for proto[0]: (int)
            _context_tl  type_list <_context>  ; param for proto[2]
            _bundle_tl   type_list <_bundle>   ; param for proto[3]
            ; HelloWorld interfaces + proto[4] param share the same content [OnClickListener]
            _oncl_tl     type_list <_oncl>
            _view_tl     type_list <_view>
            _charseq_tl  type_list <_charseq>

        ; interfaces_off for HelloWorld — list of interfaces implemented by the class
        _hello_interfaces type_list <_oncl>


        ; static=0, instance=2(btn,count), direct=1(<init>), virtual=2(onClick,onCreate)
        data_class_data
            _hello_class_data class_data_item \
                <>, \
                <<_btn_field,   ACC_PUBLIC>, <_count_field, ACC_PUBLIC>>, \
                <<_hw_init_m,  ACC_PUBLIC, _hw_init_code>>, \
                <<_hw_ocl_m,   ACC_PUBLIC, _hw_onclick_code>, \
                 <_hw_oncr_m,  ACC_PUBLIC, _hw_oncreate_code>>


        data_code
        ; ── <init>()V ─────────────────────────────────────────────
        ; p0 = this
        dex_method _hw_init_code, $01, $01, $01
            invoke_direct  _act_init_m, p0                      ; Activity.<init>
            return_void
        end dex_method

        ; ── onCreate(Bundle)V ─────────────────────────────────────
        ; registers=4, ins=2: p0=this, p1=bundle; v0=Button, v1="0"
        dex_method _hw_oncreate_code, $04, $02, $02
            invoke_super   _act_oncr_m, p0, p1                  ; Activity.onCreate
            new_instance   v0, _button_type                     ; new Button
            invoke_direct  _btn_init_m, v0, p0                  ; Button.<init>
            const_string   v1, _s0_string                       ; "0"
            invoke_virtual _btn_settt_m, v0, v1                 ; setText
            invoke_virtual _btn_setocl_m, v0, p0                ; setOnClickListener
            iput_object    v0, p0, _btn_field                   ; this.btn = btn
            invoke_virtual _act_setcv_m, p0, v0                 ; setContentView
            return_void
        end dex_method

        ; ── onClick(View)V ────────────────────────────────────────
        ; registers=4, ins=2: p0=this, p1=view
        ; v0=count(int)/result(String), v1=btn(Button)
        dex_method _hw_onclick_code, $04, $02, $02
            iget           v0, p0, _count_field                 ; count
            add_int_lit8   v0, v0, 1                            ; count++
            iput           v0, p0, _count_field                 ; this.count = count
            iget_object    v1, p0, _btn_field                   ; btn
            invoke_static  _int_tos_m, v0                       ; Integer.toString
            move_result_object v0                               ; result
            invoke_virtual _btn_settt_m, v1, v0                 ; setText
            return_void
        end dex_method

        dex_map_list

    dex_footer

end postpone
