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
    ; Sorted by MUTF-8 unsigned byte order:
    ; '0'(30) < '<'(3C) < 'I'(49) < 'L'(4C)
    ; "La..." < "Lj..." because 'a'(61) < 'j'(6A)
    ; Among "Landroid/": /a < /c < /o < /v < /w
    ; "Landroid/view/View$" < "Landroid/view/View;" because '$'(24) < ';'(3B)
    ; "Landroid/..." < "Lapp/..." because 'n'(6E) < 'p'(70)
    ; Among "Ljava/lang/": C < I < O < S
    string_ids:
        _s0_str         string_id_item _s0_data           ;  0  "0"
        _init_str       string_id_item _init_data         ;  1  "<init>"
        _I_str          string_id_item _I_data            ;  2  "I"
        _LI_str         string_id_item _LI_data           ;  3  "LI"
        _activity_str   string_id_item _activity_data     ;  4  "Landroid/app/Activity;"
        _context_str    string_id_item _context_data      ;  5  "Landroid/content/Context;"
        _bundle_str     string_id_item _bundle_data       ;  6  "Landroid/os/Bundle;"
        _oncl_str       string_id_item _oncl_data         ;  7  "Landroid/view/View$OnClickListener;"
        _view_str       string_id_item _view_data         ;  8  "Landroid/view/View;"
        _button_str     string_id_item _button_data       ;  9  "Landroid/widget/Button;"
        _hello_str      string_id_item _hello_data        ; 10  "Lapp/hello/counter_button/CounterButton;"
        _charseq_str    string_id_item _charseq_data      ; 11  "Ljava/lang/CharSequence;"
        _integer_str    string_id_item _integer_data      ; 12  "Ljava/lang/Integer;"
        _object_str     string_id_item _object_data       ; 13  "Ljava/lang/Object;"
        _string_str     string_id_item _string_data       ; 14  "Ljava/lang/String;"
        _V_str          string_id_item _V_data            ; 15  "V"
        _VL_str         string_id_item _VL_data           ; 16  "VL"
        _btn_str        string_id_item _btn_data          ; 17  "btn"
        _count_str      string_id_item _count_data        ; 18  "count"
        _onclick_str    string_id_item _onclick_data      ; 19  "onClick"
        _oncreate_str   string_id_item _oncreate_data     ; 20  "onCreate"
        _setcv_str      string_id_item _setcv_data        ; 21  "setContentView"
        _setoncl_str    string_id_item _setoncl_data      ; 22  "setOnClickListener"
        _settext_str    string_id_item _settext_data      ; 23  "setText"
        _tostring_str   string_id_item _tostring_data     ; 24  "toString"

    ; ── type_ids ──────────────────────────────────────────
    ; Sorted by string_idx ascending
    type_ids:
        _I_type        type_id_item _I_str         ; [0]  "I"       int
        _activity_type type_id_item _activity_str  ; [1]  Activity
        _context_type  type_id_item _context_str   ; [2]  Context
        _bundle_type   type_id_item _bundle_str    ; [3]  Bundle
        _oncl_type     type_id_item _oncl_str      ; [4]  OnClickListener
        _view_type     type_id_item _view_str      ; [5]  View
        _button_type   type_id_item _button_str    ; [6]  Button
        _hello_type    type_id_item _hello_str     ; [7]  HelloWorld
        _charseq_type  type_id_item _charseq_str   ; [8]  CharSequence
        _integer_type  type_id_item _integer_str   ; [9]  Integer
        _object_type   type_id_item _object_str    ; [10] Object
        _string_type   type_id_item _string_str    ; [11] String
        _void_type     type_id_item _V_str         ; [12] V

    ; ── proto_ids ─────────────────────────────────────────
    ; Sorted by (return_type_idx, param_type_idx):
    ;   return String(11) before return void(12)
    ;   void protos: ()V, (Context)V, (Bundle)V, (OnCl)V, (View)V, (CharSeq)V
    proto_ids:
        _int_str_proto  proto_id_item _LI_str, _string_type, _int_tl      ; [0] (int)String
        _void_proto     proto_id_item _V_str,  _void_type,   $00          ; [1] ()V
        _context_proto  proto_id_item _VL_str, _void_type,   _context_tl  ; [2] (Context)V
        _bundle_proto   proto_id_item _VL_str, _void_type,   _bundle_tl   ; [3] (Bundle)V
        _oncl_proto     proto_id_item _VL_str, _void_type,   _oncl_tl     ; [4] (OnClickListener)V
        _view_proto     proto_id_item _VL_str, _void_type,   _view_tl     ; [5] (View)V
        _charseq_proto  proto_id_item _VL_str, _void_type,   _charseq_tl  ; [6] (CharSequence)V

    ; ── field_ids ─────────────────────────────────────────
    ; Sorted by (class_idx=7, type_idx): Button(6) < I(0) — wait: btn field_idx=0 < count field_idx=1
    field_ids:
        _btn_field   field_id_item _hello_type, _button_type, _btn_str   ; [0] HelloWorld.btn:Button
        _count_field field_id_item _hello_type, _I_type,      _count_str ; [1] HelloWorld.count:int

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _act_init_m   method_id_item _activity_type, _void_proto,    _init_str     ; [0] Activity.<init>
        _act_oncr_m   method_id_item _activity_type, _bundle_proto,  _oncreate_str ; [1] Activity.onCreate
        _act_setcv_m  method_id_item _activity_type, _view_proto,    _setcv_str    ; [2] Activity.setContentView
        _btn_init_m   method_id_item _button_type,   _context_proto, _init_str     ; [3] Button.<init>
        _btn_setocl_m method_id_item _button_type,   _oncl_proto,    _setoncl_str  ; [4] Button.setOnClickListener
        _btn_settt_m  method_id_item _button_type,   _charseq_proto, _settext_str  ; [5] Button.setText
        _hw_init_m    method_id_item _hello_type,    _void_proto,    _init_str     ; [6] HelloWorld.<init>
        _hw_ocl_m     method_id_item _hello_type,    _view_proto,    _onclick_str  ; [7] HelloWorld.onClick
        _hw_oncr_m    method_id_item _hello_type,    _bundle_proto,  _oncreate_str ; [8] HelloWorld.onCreate
        _int_tos_m    method_id_item _integer_type,  _int_str_proto, _tostring_str ; [9] Integer.toString

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
            _hello_data     string_data_item $28, "Lapp/hello/counter_button/CounterButton;"
            _charseq_data   string_data_item $18, "Ljava/lang/CharSequence;"
            _integer_data   string_data_item $13, "Ljava/lang/Integer;"
            _object_data    string_data_item $12, "Ljava/lang/Object;"
            _string_data    string_data_item $12, "Ljava/lang/String;"
            _V_data         string_data_item $01, "V"
            _VL_data        string_data_item $02, "VL"
            _btn_data       string_data_item $03, "btn"
            _count_data     string_data_item $05, "count"
            _onclick_data   string_data_item $07, "onClick"
            _oncreate_data  string_data_item $08, "onCreate"
            _setcv_data     string_data_item $0E, "setContentView"
            _setoncl_data   string_data_item $12, "setOnClickListener"
            _settext_data   string_data_item $07, "setText"
            _tostring_data  string_data_item $08, "toString"

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

        ; <init>()V: registers=1, ins=1(v0=this), outs=1, 4 code units
        _hw_init_code    code_item $01, $01, $01, $00, $00, $04, _init_insns

        align $04

        ; onCreate(Bundle)V: registers=4, ins=2(v2=this, v3=bundle), outs=2, 22 code units
        _hw_oncreate_code code_item $04, $02, $02, $00, $00, $16, _oncreate_insns

        align $04

        ; onClick(View)V: registers=4, ins=2(v2=this, v3=view), outs=2, 16 code units
        _hw_onclick_code  code_item $04, $02, $02, $00, $00, $10, _onclick_insns

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
                <TYPE_STRING_DATA_ITEM, $19,             data_strings>,\
                <TYPE_TYPE_LIST,        $07,             data_type_lists>,\
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
; v0 = this (p0)
virtual at $00
_init_insns::
    invoke_direct  _act_init_m, v0                      ; Activity.<init>
    return_void
end virtual

; ── onCreate(Bundle)V ─────────────────────────────────────
; registers=4, ins=2: v2=this(p0), v3=bundle(p1)
; v0=Button, v1="0"
virtual at $00
_oncreate_insns::
    invoke_super   _act_oncr_m, v2, v3                  ; Activity.onCreate
    new_instance   v0, _button_type                     ; new Button
    invoke_direct  _btn_init_m, v0, v2                  ; Button.<init>
    const_string   v1, _s0_str                          ; "0"
    invoke_virtual _btn_settt_m, v0, v1                 ; setText
    invoke_virtual _btn_setocl_m, v0, v2                ; setOnClickListener
    iput_object    v0, v2, _btn_field                   ; this.btn = btn
    invoke_virtual _act_setcv_m, v2, v0                 ; setContentView
    return_void
end virtual

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
