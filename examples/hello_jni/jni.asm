format binary as "dex"
org $00

; ══════════════════════════════════════════════════════════
;  JNI Hello
;
;  public class HelloWorld extends Activity {
;      public HelloWorld() { super(); }
;      public native String answer();
;
;      public void onCreate(Bundle b) {
;          super.onCreate(b);
;          System.loadLibrary("hello");
;          TextView tv = new TextView(this);
;          tv.setText(answer());
;          setContentView(tv);
;      }
;  }
; ══════════════════════════════════════════════════════════

macro _assert_unsigned? list?*&
    iterate item?*, list
        assert (item >= $00)
    end iterate
end macro

macro ubyte? list?*&
    _assert_unsigned list
    db list
end macro

macro ushort? list?*&
    _assert_unsigned list
    dw list
end macro

macro uint? list?*&
    dd list
end macro

macro leb128? list?*&
    local value?, least?, more?
    iterate item?*, list
        value? = item
        more? = $01
        while (more)
            least? = value and $7F
            value? = value shr $07
            more? = $00
            if ~ ((value = $00) | ((value = -$01) & (least = $7F)))
                least? = least or $80
                more? = $01
            end if
            db least
        end while
    end iterate
end macro

struc _rotate_left? size?*, expression?*, amount?*
    local bitmask?, shift?, select?, offset?
    . = (expression)
    shift? = (amount) mod (size)
    select? = (($01 shl (size)) - $01)
    if (shift)
        offset? = (size) - shift
        bitmask? = ((((($01 shl amount) - $01) shl offset) and .) shr offset)
        . = (((. shl amount) and select) or bitmask)
    end if
end struc

struc sha1? payload?*
    local container?, size?, chunk?, block?, max32?, temporary?
    local h0?, h1?, h2?, h3?, h4?
    local a?, b?, c?, d?, e?, f?, k?
    h0? = $67452301
    h1? = $EFCDAB89
    h2? = $98BADCFE
    h3? = $10325476
    h4? = $C3D2E1F0
    size? = lengthof payload
    chunk? = $00
    max32? := $FFFFFFFF
    virtual at $00
    container?::
        db payload, $80
        emit (($40 - (($ + $08) mod $40)) mod $40) : $00
        dq (size * $08) bswap $08
        size? = $
    end virtual
    while (chunk < size)
        load block? : $40 from container:chunk
        repeat $10 i:$00
            local w#i?
            w#i? = ((block shr ($20 * i)) and max32) bswap $04
        end repeat
        repeat $40 i:$10, im03:$0D, im08:$08, im0E:$02, im10:$00
            local w#i?
            w#i? _rotate_left $20, (w#im03) xor (w#im08) xor (w#im0E) xor (w#im10), $01
        end repeat
        a? = h0
        b? = h1
        c? = h2
        d? = h3
        e? = h4
        repeat $50 i:$00
            if (i <= $13)
                f? = (b and c) or (((not b) and max32) and d)
                k? = $5A827999
            else if (i <= $27)
                f? = (b xor c xor d)
                k? = $6ED9EBA1
            else if (i <= $3B)
                f? = ((b and c) or (b and d) or (c and d))
                k? = $8F1BBCDC
            else
                f? = (b xor c xor d)
                k? = $CA62C1D6
            end if
            temporary? _rotate_left $20, a, $05
            temporary? = ((temporary + f + e + k + w#i) and max32)
            e? = d
            d? = c
            c? _rotate_left $20, b, $1E
            b? = a
            a? = temporary
        end repeat
        h0? = ((h0 + a) and max32)
        h1? = ((h1 + b) and max32)
        h2? = ((h2 + c) and max32)
        h3? = ((h3 + d) and max32)
        h4? = ((h4 + e) and max32)
        chunk? = chunk + $40
    end while
    . = ((h4) or (h3 shl $20) or (h2 shl $40) or (h1 shl $60) or (h0 shl $80))
end struc

struc adler32? payload?*
    local modulo?, a?, b?
    modulo? := $FFF1
    a? = $01
    b? = $00
    repeat (lengthof (payload)) i:$00
        a? = (a? + ((payload shr (i*$08)) and $FF)) mod modulo
        b? = (b? + a?) mod modulo
    end repeat
    . = (b shl $10) or a
end struc

macro align? target?*, fill?:$00
    _assert_unsigned target
    assert ((bsr target) = (bsf target))
    while ($ mod target)
        db fill
    end while
end macro

macro __define_index? base?*
    base := $00
    base.index? = $00
end macro

macro __update_index? target?*, base?*
    target = base.index?
    base.index? = base.index? + $01
end macro

macro __snapshot_index? target?*, base?*
    target := base.index?
end macro

DEX_FILE_MAGIC?  := $6465780A30333500 bswap $08
ENDIAN_CONSTANT? := $12345678
NO_INDEX?        := $FFFFFFFF
ACC_PUBLIC?      := $000001
ACC_STATIC?      := $000008
ACC_NATIVE?      := $000100
ACC_CONSTRUCTOR? := $010000

__define_index __string_id_item?
struc string_id_item? _off?*
    __update_index ., __string_id_item?
    align $04
    .string_data_off?: uint _off
end struc

__define_index __type_id_item?
struc type_id_item? _descriptor_idx?*
    __update_index ., __type_id_item?
    align $04
    .descriptor_idx?: uint _descriptor_idx
end struc

__define_index __proto_id_item?
struc proto_id_item? _shorty_idx?*, _return_type_idx?*, _parameters_off?*
    __update_index ., __proto_id_item?
    align $04
    .shorty_idx?:      uint _shorty_idx
    .return_type_idx?: uint _return_type_idx
    .parameters_off?:  uint _parameters_off
end struc

__define_index __method_id_item?
struc method_id_item? _class_idx?*, _proto_idx?*, _name_idx?*
    __update_index ., __method_id_item?
    align $04
    .class_idx?: ushort _class_idx
    .proto_idx?: ushort _proto_idx
    .name_idx?:  uint   _name_idx
end struc

__define_index __class_def_item?
struc class_def_item? _class_idx?*, _access_flags?*, _superclass_idx?*, _interfaces_off?*, \
                      _source_file_idx?*, _annotations_off?*, _class_data_off?*, _static_values_off?*
    __update_index ., __class_def_item?
    align $04
    .class_idx?:         uint _class_idx
    .access_flags?:      uint _access_flags
    .superclass_idx?:    uint _superclass_idx
    .interfaces_off?:    uint _interfaces_off
    .source_file_idx?:   uint _source_file_idx
    .annotations_off?:   uint _annotations_off
    .class_data_off?:    uint _class_data_off
    .static_values_off?: uint _static_values_off
end struc

struc type_item? _type_idx?*
    .type_idx?: ushort _type_idx
end struc

struc type_list? _size?*, _list?*
    align $04
    label .
    .size?: uint _size
    .list?:
    iterate item?, _list
        .list?.% type_item item
    end iterate
end struc

TYPE_HEADER_ITEM?      := $0000
TYPE_STRING_ID_ITEM?   := $0001
TYPE_TYPE_ID_ITEM?     := $0002
TYPE_PROTO_ID_ITEM?    := $0003
TYPE_METHOD_ID_ITEM?   := $0005
TYPE_CLASS_DEF_ITEM?   := $0006
TYPE_MAP_LIST?         := $1000
TYPE_TYPE_LIST?        := $1001
TYPE_CLASS_DATA_ITEM?  := $2000
TYPE_CODE_ITEM?        := $2001
TYPE_STRING_DATA_ITEM? := $2002

struc map_list? _size?*, _list?
    align $04
    label .
    .size?: uint _size
    .list?:
    iterate item?, _list
        .list?.% map_item item
    end iterate
end struc

struc map_item? _type?*, _size?*, _offset?*
    .type?:   ushort _type
    ushort $00
    .size?:   uint _size
    .offset?: uint _offset
end struc

struc string_data_item? _utf16_size?*, _data?&
    label .
    .utf16_size?: leb128 _utf16_size
    .data?: ubyte _data, $00
end struc

struc class_data_item? _sf?*, _if?*, _dm?*, _vm?*, \
                       _static_fields?, _instance_fields?, _direct_methods?, _virtual_methods?
    label .
    .sf?: leb128 _sf
    .if?: leb128 _if
    .dm?: leb128 _dm
    .vm?: leb128 _vm
    .static_fields?:
    iterate item?, _static_fields
        .static_fields?.% encoded_field item
    end iterate
    .instance_fields?:
    iterate item?, _instance_fields
        .instance_fields?.% encoded_field item
    end iterate
    .direct_methods?:
    iterate item?, _direct_methods
        .direct_methods?.% encoded_method item
    end iterate
    .virtual_methods?:
    iterate item?, _virtual_methods
        .virtual_methods?.% encoded_method item
    end iterate
end struc

struc encoded_field? _field_idx_diff?*, _access_flags?*
    .field_idx_diff?: leb128 _field_idx_diff
    .access_flags?:   leb128 _access_flags
end struc

struc encoded_method? _method_idx_diff?*, _access_flags?*, _code_off?*
    .method_idx_diff?: leb128 _method_idx_diff
    .access_flags?:    leb128 _access_flags
    .code_off?:        leb128 _code_off
end struc

struc code_item? _registers_size?*, _ins_size?*, _outs_size?*, _tries_size?*, \
                 _debug_info_off?*, _insns_size?*, _insns?*
    local base?, size?, opcode?
    align $04
    label .
    .registers_size?: ushort _registers_size
    .ins_size?:       ushort _ins_size
    .outs_size?:      ushort _outs_size
    .tries_size?:     ushort _tries_size
    .debug_info_off?: uint   _debug_info_off

    virtual _insns
        base? = $$
        size? = $ - $$
    end virtual

    assert ~ (size? and $01)
    size? = size? shr $01
    assert size? = _insns_size

    load opcode? : (size? shl $01) from _insns:base?
    .insns_size?: uint size?
    .insns?: db opcode
end struc

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
    ; Sorted lexicographically by MUTF-8
    string_ids:
        _init_str         string_id_item _init_data         ;  0 "<init>"
        _I_str            string_id_item _I_data            ;  1 "I"
        _L_str            string_id_item _L_data            ;  2 "L"
        _LI_str           string_id_item _LI_data           ;  3 "LI"
        _activity_str     string_id_item _activity_data     ;  4 "Landroid/app/Activity;"
        _context_str      string_id_item _context_data      ;  5 "Landroid/content/Context;"
        _bundle_str       string_id_item _bundle_data       ;  6 "Landroid/os/Bundle;"
        _view_str         string_id_item _view_data         ;  7 "Landroid/view/View;"
        _textview_str     string_id_item _textview_data     ;  8 "Landroid/widget/TextView;"
        _hello_str        string_id_item _hello_data        ;  9 "Lapp/hello/jni/HelloWorld;"
        _charseq_str      string_id_item _charseq_data      ; 10 "Ljava/lang/CharSequence;"
        _integer_str      string_id_item _integer_data      ; 11 "Ljava/lang/Integer;"
        _object_str       string_id_item _object_data       ; 12 "Ljava/lang/Object;"
        _string_str       string_id_item _string_data       ; 13 "Ljava/lang/String;"
        _system_str       string_id_item _system_data       ; 14 "Ljava/lang/System;"
        _V_str            string_id_item _V_data            ; 15 "V"
        _VL_str           string_id_item _VL_data           ; 16 "VL"
        _answer_str       string_id_item _answer_data       ; 17 "answer"
        _hello_lib_str    string_id_item _hello_lib_data    ; 18 "hello"
        _loadlibrary_str  string_id_item _loadlibrary_data  ; 20 "loadLibrary"
        _oncreate_str     string_id_item _oncreate_data     ; 21 "onCreate"
        _setcv_str        string_id_item _setcv_data        ; 22 "setContentView"
        _settext_str      string_id_item _settext_data      ; 23 "setText"
        _tostring_str     string_id_item _tostring_data     ; 24 "toString"

    ; ── type_ids ──────────────────────────────────────────
    ; Sorted by string_idx ascending
    type_ids:
        _I_type        type_id_item _I_str          ; [0]  int
        _activity_type type_id_item _activity_str   ; [1]  Activity
        _context_type  type_id_item _context_str    ; [2]  Context
        _bundle_type   type_id_item _bundle_str     ; [3]  Bundle
        _view_type     type_id_item _view_str       ; [4]  View
        _textview_type type_id_item _textview_str   ; [5]  TextView
        _hello_type    type_id_item _hello_str      ; [6]  HelloWorld
        _charseq_type  type_id_item _charseq_str    ; [7]  CharSequence
        _integer_type  type_id_item _integer_str    ; [8]  Integer
        _object_type   type_id_item _object_str     ; [9]  Object
        _string_type   type_id_item _string_str     ; [10] String
        _system_type   type_id_item _system_str     ; [11] System
        _void_type     type_id_item _V_str          ; [12] V

    ; ── proto_ids ─────────────────────────────────────────
    proto_ids:
        _I_proto          proto_id_item _I_str,  _I_type,      $00          ; [0] ()I
        _string_ret_proto proto_id_item _L_str,  _string_type, $00          ; [1] ()String
        _I_str_proto      proto_id_item _LI_str, _string_type, _I_tl        ; [2] (I)String
        _void_proto       proto_id_item _V_str,  _void_type,   $00          ; [3] ()V
        _context_proto    proto_id_item _VL_str, _void_type,   _context_tl  ; [4] (Context)V
        _bundle_proto     proto_id_item _VL_str, _void_type,   _bundle_tl   ; [5] (Bundle)V
        _view_proto       proto_id_item _VL_str, _void_type,   _view_tl     ; [6] (View)V
        _charseq_proto    proto_id_item _VL_str, _void_type,   _charseq_tl  ; [7] (CharSequence)V
        _string_proto     proto_id_item _VL_str, _void_type,   _string_tl   ; [8] (String)V

    ; ── method_ids ────────────────────────────────────────
    ; Sorted by (class_idx, proto_idx, name_idx)
    method_ids:
        _act_init_m    method_id_item _activity_type, _void_proto,       _init_str        ; [0]
        _act_oncr_m    method_id_item _activity_type, _bundle_proto,     _oncreate_str    ; [1]
        _act_setcv_m   method_id_item _activity_type, _view_proto,       _setcv_str       ; [2]
        _tv_init_m     method_id_item _textview_type, _context_proto,    _init_str        ; [3]
        _tv_settt_m    method_id_item _textview_type, _charseq_proto,    _settext_str     ; [4]
        _hw_init_m     method_id_item _hello_type,    _void_proto,       _init_str        ; [5]
        _hw_answer_m   method_id_item _hello_type,    _string_ret_proto, _answer_str      ; [6]
        _hw_oncr_m     method_id_item _hello_type,    _bundle_proto,     _oncreate_str    ; [7]
        _int_tos_m     method_id_item _integer_type,  _I_str_proto,      _tostring_str    ; [8]
        _sys_loadlib_m method_id_item _system_type,   _string_proto,     _loadlibrary_str ; [9]

    class_defs:
        _hello_class class_def_item \
            _hello_type, ACC_PUBLIC, _activity_type, \
            $00, NO_INDEX, $00, _hello_class_data, $00

    data:

        data_strings:
            _init_data        string_data_item $06, "<init>"
            _I_data           string_data_item $01, "I"
            _L_data           string_data_item $01, "L"
            _LI_data          string_data_item $02, "LI"
            _activity_data    string_data_item $16, "Landroid/app/Activity;"
            _context_data     string_data_item $19, "Landroid/content/Context;"
            _bundle_data      string_data_item $13, "Landroid/os/Bundle;"
            _view_data        string_data_item $13, "Landroid/view/View;"
            _textview_data    string_data_item $19, "Landroid/widget/TextView;"
            _hello_data       string_data_item $1A, "Lapp/hello/jni/HelloWorld;"
            _charseq_data     string_data_item $18, "Ljava/lang/CharSequence;"
            _integer_data     string_data_item $13, "Ljava/lang/Integer;"
            _object_data      string_data_item $12, "Ljava/lang/Object;"
            _string_data      string_data_item $12, "Ljava/lang/String;"
            _system_data      string_data_item $12, "Ljava/lang/System;"
            _V_data           string_data_item $01, "V"
            _VL_data          string_data_item $02, "VL"
            _answer_data      string_data_item $06, "answer"
            _hello_lib_data   string_data_item $05, "hello"
            _loadlibrary_data string_data_item $0B, "loadLibrary"
            _oncreate_data    string_data_item $08, "onCreate"
            _setcv_data       string_data_item $0E, "setContentView"
            _settext_data     string_data_item $07, "setText"
            _tostring_data    string_data_item $08, "toString"

        align $04

        data_type_lists:
            _I_tl       type_list $01, <_I_type>
            _context_tl type_list $01, <_context_type>
            _bundle_tl  type_list $01, <_bundle_type>
            _view_tl    type_list $01, <_view_type>
            _charseq_tl type_list $01, <_charseq_type>
            _string_tl  type_list $01, <_string_type>

        align $04

        ; static=0, instance=0, direct=1(<init>), virtual=2(answer[native],onCreate)
        data_class_data:
            _hello_class_data class_data_item $00, $00, $01, $02, \
                <>, <>, \
                <<_hw_init_m,   ACC_PUBLIC or ACC_CONSTRUCTOR, _hw_init_code>>, \
                <<_hw_answer_m, ACC_PUBLIC or ACC_NATIVE,      $00>, \
                 <$01,          ACC_PUBLIC,                    _hw_oncreate_code>>

        align $04

        ; <init>()V: registers=1, ins=1(v0=this), outs=1, 4 code units
        _hw_init_code code_item $01, $01, $01, $00, $00, $04, _init_insns

        align $04

        ; onCreate(Bundle)V: registers=4, ins=2(v2=this, v3=bundle), outs=2, 24 code units
        _hw_oncreate_code code_item $04, $02, $02, $00, $00, $18, _oncreate_insns

        align $04

        data_map:
            _map map_list $0B, <\
                <TYPE_HEADER_ITEM,      $01,             header>,\
                <TYPE_STRING_ID_ITEM,   string_ids_size, string_ids>,\
                <TYPE_TYPE_ID_ITEM,     type_ids_size,   type_ids>,\
                <TYPE_PROTO_ID_ITEM,    proto_ids_size,  proto_ids>,\
                <TYPE_METHOD_ID_ITEM,   method_ids_size, method_ids>,\
                <TYPE_CLASS_DEF_ITEM,   class_defs_size, class_defs>,\
                <TYPE_STRING_DATA_ITEM, $19,             data_strings>,\
                <TYPE_TYPE_LIST,        $06,             data_type_lists>,\
                <TYPE_CLASS_DATA_ITEM,  $01,             data_class_data>,\
                <TYPE_CODE_ITEM,        $02,             _hw_init_code>,\
                <TYPE_MAP_LIST,         $01,             data_map>\
            >

        align $04

    link_data:
    header_end:

    __snapshot_index string_ids_size, __string_id_item
    __snapshot_index type_ids_size,   __type_id_item
    __snapshot_index proto_ids_size,  __proto_id_item
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
    ; invoke-direct {v0}, Activity.<init>()V
    dw $1070, _act_init_m, $0000
    ; return-void
    dw $000E
end virtual

; ── onCreate(Bundle)V ─────────────────────────────────────
; registers=4, ins=2: v2=this(p0), v3=bundle(p1)
; v0=temp/result, v1=TextView
virtual at $00
_oncreate_insns::
    ; invoke-super {v2,v3}, Activity.onCreate(Bundle)V
    dw $206F, _act_oncr_m, $0032

    ; const-string v0, "hello"
    dw $001A, _hello_lib_str

    ; invoke-static {v0}, System.loadLibrary(String)V
    dw $1071, _sys_loadlib_m, $0000

    ; new-instance v1, TextView
    dw $0122, _textview_type

    ; invoke-direct {v1,v2}, TextView.<init>(Context)V
    ; 35c: A=2, C=v1, D=v2 → word3=(2<<4)|1=$0021
    dw $2070, _tv_init_m, $0021

    ; invoke-virtual {v2}, this.answer()String
    ; 35c: A=1, C=v2 → word1=$106E, word3=$0002
    dw $106E, _hw_answer_m, $0002

    ; move-result-object v0
    dw $000C

    ; invoke-virtual {v1,v0}, TextView.setText(CharSequence)V
    ; 35c: A=2, C=v1, D=v0 → word3=(0<<4)|1=$0001
    dw $206E, _tv_settt_m, $0001

    ; invoke-virtual {v2,v1}, Activity.setContentView(View)V
    ; 35c: A=2, C=v2, D=v1 → word3=(1<<4)|2=$0012
    dw $206E, _act_setcv_m, $0012

    ; return-void
    dw $000E
end virtual
