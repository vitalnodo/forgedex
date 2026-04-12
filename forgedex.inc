; ══════════════════════════════════════════════════════════
;  forgedex.inc — shared DSL for hand-forging DEX files
;
;  Include this file at the top of every example:
;      format binary as "dex"
;      org $00
;      include '../../forgedex.inc'
;
;  Provides:
;    • Primitive type macros (ubyte, ushort, uint, byte, short, int, long, ulong, leb128)
;    • Alignment macro
;    • SHA-1 and Adler-32 computed at assembly time
;    • All DEX structure definitions (string_id_item, type_id_item, etc.)
;    • Access flags and type constants
;    • Index tracking system (__define_index / __update_index / __snapshot_index)
; ══════════════════════════════════════════════════════════

; ── Assertion helper ──────────────────────────────────────

macro _assert_unsigned? list?*&
    iterate item?*, list
        assert (item >= $00)
    end iterate
end macro

; ── Primitive type macros ─────────────────────────────────

macro byte? list?*&
    db list
end macro

macro ubyte? list?*&
    _assert_unsigned list
    db list
end macro

macro short? list?*&
    dw list
end macro

macro ushort? list?*&
    _assert_unsigned list
    dw list
end macro

macro int? list?*&
    dd list
end macro

macro uint? list?*&
    dd list
end macro

macro long? list?*&
    dq list
end macro

macro ulong? list?*&
    _assert_unsigned list
    dq list
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

; ── Bit rotation ──────────────────────────────────────────

struc _rotate_left? size?*, expression?*, amount?*
    local bitmask?, shift?, select?, offset?
    _assert_unsigned size, amount
    . = (expression)
    shift? = (amount) mod (size)
    select? = (($01 shl (size)) - $01)
    if (shift)
        offset? = (size) - shift
        bitmask? = ((((($01 shl amount) - $01) shl offset) and .) shr offset)
        . = (((. shl amount) and select) or bitmask)
    end if
end struc

; ── SHA-1 (assembly-time) ─────────────────────────────────

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

; ── Adler-32 (assembly-time) ──────────────────────────────

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

; ── Alignment ─────────────────────────────────────────────

macro align? target?*, fill?:$00
    _assert_unsigned target
    assert ((bsr target) = (bsf target))
    while ($ mod target)
        db fill
    end while
end macro

; ── Index tracking system ─────────────────────────────────

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

; ── DEX constants ─────────────────────────────────────────

DEX_FILE_MAGIC?          := $6465780A30333500 bswap $08
ENDIAN_CONSTANT?         := $12345678
REVERSE_ENDIAN_CONSTANT? := $78563412
NO_INDEX?                := $FFFFFFFF

; ── Access flags ──────────────────────────────────────────

ACC_PUBLIC?       := $000001
ACC_PRIVATE?      := $000002
ACC_PROTECTED?    := $000004
ACC_STATIC?       := $000008
ACC_FINAL?        := $000010
ACC_NATIVE?       := $000100
ACC_INTERFACE?    := $000200
ACC_ABSTRACT?     := $000400
ACC_ANNOTATION?   := $002000
ACC_ENUM?         := $004000
ACC_CONSTRUCTOR?  := $010000

; ── Map type constants ────────────────────────────────────

TYPE_HEADER_ITEM?          := $0000
TYPE_STRING_ID_ITEM?       := $0001
TYPE_TYPE_ID_ITEM?         := $0002
TYPE_PROTO_ID_ITEM?        := $0003
TYPE_FIELD_ID_ITEM?        := $0004
TYPE_METHOD_ID_ITEM?       := $0005
TYPE_CLASS_DEF_ITEM?       := $0006
TYPE_CALL_SITE_ID_ITEM?    := $0007
TYPE_METHOD_HANDLE_ITEM?   := $0008
TYPE_MAP_LIST?             := $1000
TYPE_TYPE_LIST?            := $1001
TYPE_CLASS_DATA_ITEM?      := $2000
TYPE_CODE_ITEM?            := $2001
TYPE_STRING_DATA_ITEM?     := $2002

; ── DEX structure definitions ─────────────────────────────

__define_index __string_id_item?

struc string_id_item? _string_data_off?*
    __update_index ., __string_id_item?
    align $04
    .string_data_off?: uint _string_data_off
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

__define_index __field_id_item?

struc field_id_item? _class_idx?*, _type_idx?*, _name_idx?*
    __update_index ., __field_id_item?
    align $04
    .class_idx?: ushort _class_idx
    .type_idx?:  ushort _type_idx
    .name_idx?:  uint   _name_idx
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

struc class_def_item?           _class_idx?*, _access_flags?*, _superclass_idx?*, _interfaces_off?*,\
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

struc class_data_item?          _static_fields_size?*, _instance_fields_size?*, _direct_methods_size?*, _virtual_methods_size?*,\
                                _static_fields?, _instance_fields?, _direct_methods?, _virtual_methods?
    label .
    .static_fields_size?:   leb128 _static_fields_size
    .instance_fields_size?: leb128 _instance_fields_size
    .direct_methods_size?:  leb128 _direct_methods_size
    .virtual_methods_size?: leb128 _virtual_methods_size
    .static_fields?:
    iterate static_field?, _static_fields
        .static_fields?.% encoded_field static_field
    end iterate
    .instance_fields?:
    iterate instance_field?, _instance_fields
        .instance_fields?.% encoded_field instance_field
    end iterate
    .direct_methods?:
    iterate direct_method?, _direct_methods
        .direct_methods?.% encoded_method direct_method
    end iterate
    .virtual_methods?:
    iterate virtual_method?, _virtual_methods
        .virtual_methods?.% encoded_method virtual_method
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

struc code_item? _registers_size?*, _ins_size?*, _outs_size?*, _tries_size?*, _debug_info_off?*, _insns_size?*, _insns?*
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
    assert ~ (size and $01)
    size? = size shr $01
    if (size > _insns_size)
        size? = _insns_size
    end if
    load opcode? : (size shl $01) from _insns:base
    .insns_size?: uint size
    .insns?: db opcode
    if (_tries_size)
        align $04
        .tries?:
        .handlers?:
    end if
end struc

struc try_item? _start_addr?*, _insn_count?*, _handler_off?*
    .start_addr?:  uint   _start_addr
    .insn_count?:  ushort _insn_count
    .handler_off?: ushort _handler_off
end struc

; ── Register aliases ──────────────────────────────────────
 
v0 = 0
v1 = 1
v2 = 2
v3 = 3
v4 = 4
v5 = 5
v6 = 6
v7 = 7
v8 = 8
v9 = 9
v10 = 10
v11 = 11
v12 = 12
v13 = 13
v14 = 14
v15 = 15
 
; ── Dalvik instruction formats ────────────────────────────
 
macro __fmt_10x? op?*
    dw op
end macro
 
macro __fmt_10t? op?*, offset?*
    dw ((offset and $FF) shl 8) or op
end macro
 
macro __fmt_11x? op?*, a?*
    dw (a shl 8) or op
end macro
 
macro __fmt_11n? op?*, a?*, lit?*
    dw ((lit and $0F) shl 12) or (a shl 8) or op
end macro
 
macro __fmt_21c? op?*, a?*, idx?*
    dw (a shl 8) or op, idx
end macro
 
macro __fmt_21t? op?*, a?*, offset?*
    dw (a shl 8) or op, offset
end macro
 
macro __fmt_22c? op?*, a?*, b?*, idx?*
    dw (b shl 12) or (a shl 8) or op, idx
end macro
 
macro __fmt_22t? op?*, a?*, b?*, offset?*
    dw (b shl 12) or (a shl 8) or op, offset
end macro
 
macro __fmt_22b? op?*, a?*, b?*, lit?*
    dw (a shl 8) or op, ((lit and $FF) shl 8) or b
end macro
 
macro __fmt_35c? op?*, idx?*, regs?*&
    local count?, c?, d?, e?, f?, g?
    c? = 0
    d? = 0
    e? = 0
    f? = 0
    g? = 0
    count? = 0
    iterate reg?, regs
        count? = count? + 1
        match =1, %
            c? = reg
        end match
        match =2, %
            d? = reg
        end match
        match =3, %
            e? = reg
        end match
        match =4, %
            f? = reg
        end match
        match =5, %
            g? = reg
        end match
    end iterate
    dw (count? shl 12) or (g? shl 8) or op, idx, (f? shl 12) or (e? shl 8) or (d? shl 4) or c?
end macro
 
; ── Dalvik instructions ───────────────────────────────────
 
macro return_void?
    __fmt_10x $0E
end macro
 
macro move_result_object? a?*
    __fmt_11x $0C, a
end macro
 
macro const_4? a?*, lit?*
    __fmt_11n $12, a, lit
end macro
 
macro goto? offset?*
    __fmt_10t $28, offset
end macro
 
macro const_string? a?*, idx?*
    __fmt_21c $1A, a, idx
end macro
 
macro new_instance? a?*, idx?*
    __fmt_21c $22, a, idx
end macro
 
macro sget_object? a?*, idx?*
    __fmt_21c $62, a, idx
end macro
 
macro if_eqz? a?*, offset?*
    __fmt_21t $38, a, offset
end macro
 
macro iget? a?*, b?*, idx?*
    __fmt_22c $52, a, b, idx
end macro
 
macro iget_object? a?*, b?*, idx?*
    __fmt_22c $54, a, b, idx
end macro
 
macro iput? a?*, b?*, idx?*
    __fmt_22c $59, a, b, idx
end macro
 
macro iput_object? a?*, b?*, idx?*
    __fmt_22c $5B, a, b, idx
end macro
 
macro if_eq? a?*, b?*, offset?*
    __fmt_22t $32, a, b, offset
end macro
 
macro add_int_lit8? a?*, b?*, lit?*
    __fmt_22b $D8, a, b, lit
end macro
 
macro invoke_virtual? method?*, regs?*&
    __fmt_35c $6E, method, regs
end macro
 
macro invoke_super? method?*, regs?*&
    __fmt_35c $6F, method, regs
end macro
 
macro invoke_direct? method?*, regs?*&
    __fmt_35c $70, method, regs
end macro
 
macro invoke_static? method?*, regs?*&
    __fmt_35c $71, method, regs
end macro
