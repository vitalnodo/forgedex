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

; Convenience wrapper over proto_id_item: the shorty name and the
; return type are given as defstr/deftype bases, so the _string and
; _type suffixes don't have to be repeated at every call site.
; The parameters offset is passed through unchanged — $00 for a
; no-arg method, or a type_list label otherwise.
struc proto? _shorty?*, _return?*, _parameters_off?*
    . proto_id_item _shorty#_string, _return#_type, _parameters_off
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

; Convenience wrappers: same as method_id_item / field_id_item but
; the name is given as the defstr base (e.g. _init) rather than
; the generated string label (_init_string). Saves a '_string'
; suffix at every call site.
;
; Both also assert that the entries are added in the sorted order
; DEX requires: method_ids by (class_idx, proto_idx, name_idx), and
; field_ids by (class_idx, type_idx, name_idx). The sort key is
; packed into a single 64-bit int so a plain '>' comparison works
; lexicographically: type/class idx in bits [48..63], second field
; in [32..47], name idx in [0..31]. All indices fit (16-bit for
; type/class/proto, 32-bit for name).
__method_id_prev_key? = -1
__field_id_prev_key?  = -1

struc method_id? _class_idx?*, _proto_idx?*, _name?*
    local key?
    key? = (_class_idx shl $30) or (_proto_idx shl $20) or _name#_string
    assert key? > __method_id_prev_key?      ; method_ids must be strictly sorted
    __method_id_prev_key? = key?
    . method_id_item _class_idx, _proto_idx, _name#_string
end struc

struc field_id? _class_idx?*, _type_idx?*, _name?*
    local key?
    key? = (_class_idx shl $30) or (_type_idx shl $20) or _name#_string
    assert key? > __field_id_prev_key?       ; field_ids must be strictly sorted
    __field_id_prev_key? = key?
    . field_id_item _class_idx, _type_idx, _name#_string
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

; Common-case wrapper: a plain public class with no source file,
; no annotations, and no static-initializer values. Covers every
; example in this repo and most hand-written Android activities.
;   public_class _class_type, _super_type, _interfaces, _class_data
; Pass $00 for _interfaces if the class implements no interfaces.
struc public_class? _class_idx?*, _superclass_idx?*, _interfaces_off?*, _class_data_off?*
    . class_def_item _class_idx, ACC_PUBLIC, _superclass_idx, _interfaces_off,\
                     NO_INDEX, $00, _class_data_off, $00
end struc

struc type_item? _type_idx?*
    .type_idx?: ushort _type_idx
end struc

__type_list_count? = 0

struc type_list? _bases?*
    local n?
    __type_list_count? = __type_list_count? + 1
    n? = 0
    iterate base?, _bases
        n? = n? + 1
    end iterate
    align $04
    label .
    .size?: uint n?
    .list?:
    iterate base?, _bases
        .list?.% type_item base#_type
    end iterate
end struc

struc string_data_item? _utf16_size?*, _data?&
    label .
    .utf16_size?: leb128 _utf16_size
    .data?: ubyte _data, $00
end struc

struc class_data_item?  _static_fields?, _instance_fields?, _direct_methods?, _virtual_methods?
    local n_static?, n_instance?, n_direct?, n_virtual?
    local prev?, i?, _id?, _flags?, _code?
    n_static?   = 0
    n_instance? = 0
    n_direct?   = 0
    n_virtual?  = 0
    iterate item?, _static_fields
        n_static? = n_static? + 1
    end iterate
    iterate item?, _instance_fields
        n_instance? = n_instance? + 1
    end iterate
    iterate item?, _direct_methods
        n_direct? = n_direct? + 1
    end iterate
    iterate item?, _virtual_methods
        n_virtual? = n_virtual? + 1
    end iterate
    label .
    .static_fields_size?:   leb128 n_static?
    .instance_fields_size?: leb128 n_instance?
    .direct_methods_size?:  leb128 n_direct?
    .virtual_methods_size?: leb128 n_virtual?
    ; Each encoded_field / encoded_method stores a delta against the
    ; previous entry's absolute id (reset per list). Users pass absolute
    ; method_id / field_id labels throughout; the delta is computed here.
    .static_fields?:
    prev? = 0
    iterate field?, _static_fields
        i? = 0
        iterate x?, field
            if i? = 0
                _id? = x
            else if i? = 1
                _flags? = x
            end if
            i? = i? + 1
        end iterate
        .static_fields?.% encoded_field (_id? - prev?), _flags?
        prev? = _id?
    end iterate
    .instance_fields?:
    prev? = 0
    iterate field?, _instance_fields
        i? = 0
        iterate x?, field
            if i? = 0
                _id? = x
            else if i? = 1
                _flags? = x
            end if
            i? = i? + 1
        end iterate
        .instance_fields?.% encoded_field (_id? - prev?), _flags?
        prev? = _id?
    end iterate
    .direct_methods?:
    prev? = 0
    iterate method?, _direct_methods
        i? = 0
        iterate x?, method
            if i? = 0
                _id? = x
            else if i? = 1
                _flags? = x
            else if i? = 2
                _code? = x
            end if
            i? = i? + 1
        end iterate
        .direct_methods?.% encoded_method (_id? - prev?), _flags?, _code?
        prev? = _id?
    end iterate
    .virtual_methods?:
    prev? = 0
    iterate method?, _virtual_methods
        i? = 0
        iterate x?, method
            if i? = 0
                _id? = x
            else if i? = 1
                _flags? = x
            else if i? = 2
                _code? = x
            end if
            i? = i? + 1
        end iterate
        .virtual_methods?.% encoded_method (_id? - prev?), _flags?, _code?
        prev? = _id?
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

__code_item_count? = 0

struc code_item? _registers_size?*, _ins_size?*, _outs_size?*, _tries_size?*, _debug_info_off?*, _insns_size?*, _insns?*
    local base?, size?, opcode?
    __code_item_count? = __code_item_count? + 1
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
;
;  Each __fmt_XX macro encodes one Dalvik instruction format.
;  Range assertions catch the common mistakes early (wrong register
;  width, out-of-range literal, out-of-range branch offset) — fasmg
;  will fail with a clear error at the offending line instead of
;  silently producing a DEX that Android's verifier rejects.
;
;  Register naming: 4-bit register fields hold v0..v15 only; 8-bit
;  fields hold v0..v255. 16-bit fields (not used by these formats)
;  would hold v0..v65535.

macro __fmt_10x? op?*
    dw op
end macro

macro __fmt_10t? op?*, target?*
    local here, offset
    here = $
    offset = (target - here) / 2
    assert (offset >= -$80) & (offset <= $7F)       ; signed 8-bit branch
    assert ((target - here) mod 2) = 0              ; codeunit-aligned
    dw ((offset and $FF) shl 8) or op
end macro

macro __fmt_11x? op?*, a?*
    assert (a >= $00) & (a <= $FF)                  ; 8-bit register
    dw (a shl 8) or op
end macro

macro __fmt_11n? op?*, a?*, lit?*
    assert (a >= $00) & (a <= $0F)                  ; 4-bit register (v0..v15)
    assert (lit >= -$08) & (lit <= $07)             ; signed 4-bit literal
    dw ((lit and $0F) shl 12) or (a shl 8) or op
end macro

macro __fmt_21c? op?*, a?*, idx?*
    assert (a >= $00) & (a <= $FF)                  ; 8-bit register
    assert (idx >= $00) & (idx <= $FFFF)            ; 16-bit pool index
    dw (a shl 8) or op, idx
end macro

macro __fmt_21t? op?*, a?*, target?*
    local here, offset
    here = $
    offset = (target - here) / 2
    assert (a >= $00) & (a <= $FF)                  ; 8-bit register
    assert (offset >= -$8000) & (offset <= $7FFF)   ; signed 16-bit branch
    assert ((target - here) mod 2) = 0              ; codeunit-aligned
    dw (a shl 8) or op, offset
end macro

macro __fmt_22c? op?*, a?*, b?*, idx?*
    assert (a >= $00) & (a <= $0F)                  ; 4-bit register
    assert (b >= $00) & (b <= $0F)                  ; 4-bit register
    assert (idx >= $00) & (idx <= $FFFF)            ; 16-bit pool index
    dw (b shl 12) or (a shl 8) or op, idx
end macro

macro __fmt_22t? op?*, a?*, b?*, target?*
    local here, offset
    here = $
    offset = (target - here) / 2
    assert (a >= $00) & (a <= $0F)                  ; 4-bit register
    assert (b >= $00) & (b <= $0F)                  ; 4-bit register
    assert (offset >= -$8000) & (offset <= $7FFF)   ; signed 16-bit branch
    assert ((target - here) mod 2) = 0              ; codeunit-aligned
    dw (b shl 12) or (a shl 8) or op, offset
end macro

macro __fmt_22b? op?*, a?*, b?*, lit?*
    assert (a >= $00) & (a <= $FF)                  ; 8-bit register
    assert (b >= $00) & (b <= $FF)                  ; 8-bit register
    assert (lit >= -$80) & (lit <= $7F)             ; signed 8-bit literal
    dw (a shl 8) or op, ((lit and $FF) shl 8) or b
end macro

macro __fmt_35c? op?*, idx?*, regs?*&
    local count?, c?, d?, e?, f?, g?
    assert (idx >= $00) & (idx <= $FFFF)            ; 16-bit pool index
    c? = 0
    d? = 0
    e? = 0
    f? = 0
    g? = 0
    count? = 0
    iterate reg?, regs
        assert (reg >= $00) & (reg <= $0F)          ; each register is 4-bit
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
    assert count <= 5                               ; 35c: up to 5 registers
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
 
macro goto? target?*
    __fmt_10t $28, target
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
 
macro if_eqz? a?*, target?*
    __fmt_21t $38, a, target
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
 
macro if_eq? a?*, b?*, target?*
    __fmt_22t $32, a, b, target
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

struc defstring? _data?*&
    local utf16len?, i?, b?, b1?, b2?, b3?, cp?, hi?, lo?
    utf16len? = 0
    i? = 0
    while i < lengthof _data
        b? = (_data shr (i * 8)) and $FF
        if b < $80
            utf16len? = utf16len? + 1
            i? = i + 1
        else if b < $E0
            utf16len? = utf16len? + 1
            i? = i + 2
        else if b < $F0
            utf16len? = utf16len? + 1
            i? = i + 3
        else
            utf16len? = utf16len? + 2
            i? = i + 4
        end if
    end while
    label .
    .utf16_size?: leb128 utf16len?
    ; emit MUTF-8 data: convert 4-byte UTF-8 to surrogate pairs
    i? = 0
    while i < lengthof _data
        b? = (_data shr (i * 8)) and $FF
        if b < $F0
            db b
            i? = i + 1
        else
            ; decode 4-byte UTF-8 to codepoint
            b1? = (_data shr ((i + 1) * 8)) and $FF
            b2? = (_data shr ((i + 2) * 8)) and $FF
            b3? = (_data shr ((i + 3) * 8)) and $FF
            cp? = ((b and $07) shl 18) or ((b1 and $3F) shl 12) or ((b2 and $3F) shl 6) or (b3 and $3F)
            ; encode as CESU-8 surrogate pair
            hi? = $D800 or ((cp - $10000) shr 10)
            lo? = $DC00 or ((cp - $10000) and $3FF)
            ; high surrogate as 3-byte CESU-8
            db $E0 or (hi shr 12)
            db $80 or ((hi shr 6) and $3F)
            db $80 or (hi and $3F)
            ; low surrogate as 3-byte CESU-8
            db $E0 or (lo shr 12)
            db $80 or ((lo shr 6) and $3F)
            db $80 or (lo and $3F)
            i? = i + 4
        end if
    end while
    db $00
end struc

macro emit_checksums?
    __snapshot_index string_ids_size, __string_id_item
    __snapshot_index type_ids_size,   __type_id_item
    __snapshot_index proto_ids_size,  __proto_id_item
    __snapshot_index field_ids_size,  __field_id_item
    __snapshot_index method_ids_size, __method_id_item
    __snapshot_index class_defs_size, __class_def_item
    type_list_count = __type_list_count
    code_item_count = __code_item_count

    load __sha1_payload : ($ - header_sha1) from __global : header_sha1
    __sha1_result sha1 __sha1_payload
    store __sha1_result : $14 at __global : header_adler32

    load __adler32_payload : ($ - header_adler32) from __global : header_adler32
    __adler32_result adler32 __adler32_payload
    store __adler32_result : $04 at __global : header_magic_end
end macro

; ── DEX file scaffolding ─────────────────────────────────
;
;  Usage inside postpone block:
;
;      postpone
;      __global::
;          dex_header
;
;          string_ids:
;              ...
;          type_ids:
;              ...
;          ...
;          data:
;              ...
;
;          dex_footer
;      end postpone
;
;  field_ids is optional: if the label is not defined,
;  the header emits zeroes for field_ids_size / field_ids_off.

macro dex_header?
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
        if defined field_ids
            uint field_ids_size
            uint (field_ids - header)
        else
            uint $00
            uint $00
        end if
        uint method_ids_size
        uint (method_ids  - header)
        uint class_defs_size
        uint (class_defs  - header)
        uint (link_data   - data)           ; data_size
        uint (data        - header)         ; data_off
end macro

macro dex_footer?
    link_data:
    header_end:

    emit_checksums
end macro

; ── String list helpers ──────────────────────────────────
;
;  Usage:
;      macro _strings
;          defstr     _init,     "<init>"
;          defstrtype _activity, "Landroid/app/Activity;"   ; marked as a type
;          defstrtype _V,        "V"
;      end macro
;
;      string_ids: emit_string_ids _strings
;      type_ids:   emit_type_ids   _strings
;      data_strings:
;                  emit_string_data _strings
;
;  defstrtype is defstr + a mark; emit_type_ids replays the callback
;  and emits a type_id_item only for marked strings, in declaration
;  order. Because DEX requires type_ids sorted by descriptor_idx
;  (= index into the already-sorted string pool), users must keep
;  defstrtype entries in the same relative order as the underlying
;  strings — which is already the natural thing to do, and matches
;  the previous manual 'deftype' listing order.

__defstr_mode = 0
; __defstr_mode values:
;   0 — emit string_id_item
;   1 — emit string_data (UTF-8 / MUTF-8 bytes with leb128 length)
;   2 — emit type_id_item for marked strings (others ignored)

macro defstr? base?*, text?*&
    if __defstr_mode = 0
        base#_string string_id_item base#_data
    else if __defstr_mode = 1
        base#_data defstring text
    end if
    ; mode 2: plain defstr contributes nothing to type_ids
end macro

macro defstrtype? base?*, text?*&
    if __defstr_mode = 0
        base#_string string_id_item base#_data
    else if __defstr_mode = 1
        base#_data defstring text
    else if __defstr_mode = 2
        base#_type type_id_item base#_string
    end if
end macro

macro emit_string_ids? callback?*
    __defstr_mode = 0
    callback
end macro

macro emit_string_data? callback?*
    __defstr_mode = 1
    callback
end macro

macro emit_type_ids? callback?*
    __defstr_mode = 2
    callback
end macro

; ── Data section labels ───────────────────────────────────
;
;  The data section needs specific labels (data_strings,
;  data_type_lists, data_class_data, data_code, data_map) so that
;  dex_map_list and dex_header can reference them. Each of these
;  macros emits that label at a 4-byte-aligned offset, which is
;  what the DEX layout requires.
;
;  Note: data_code is deliberately NOT aligned here — the code_item
;  struc aligns itself at the start of its own body, and the label
;  points at the first code_item, not at padding before it.

macro data_strings? callback?
    align $04
    data_strings:
    match cb, callback
        emit_string_data cb
    end match
end macro

macro data_type_lists?
    align $04
    data_type_lists:
end macro

macro data_class_data?
    align $04
    data_class_data:
end macro

macro data_code?
    align $04
    data_code:
end macro

; ── Type helper ──────────────────────────────────────────
;
;  deftype is kept for backward compatibility with examples that
;  declare strings via plain defstr and list types manually.

macro deftype? base?*
    base#_type type_id_item base#_string
end macro

; ── Automatic map_list ───────────────────────────────────
;
;  Requires standard labels in data section:
;      data_strings:      (before first defstring/emit_string_data)
;      data_type_lists:   (before first type_list)
;      data_class_data:   (before first class_data_item)
;      data_code:         (before first code_item)
;      data_map:          (generated by this macro)

macro emit_map_item? _type?*, _size?*, _offset?*
    ushort _type
    ushort $00
    uint _size
    uint _offset
end macro

macro dex_map_list?
    local count
    ; Baseline always-present entries: HEADER, STRING_ID, TYPE_ID, PROTO_ID,
    ; METHOD_ID, CLASS_DEF, STRING_DATA, CLASS_DATA = 8.
    ; Plus: field_ids (optional), type_list (optional), code_item (optional),
    ; and the map_list entry itself.
    count = $08
    if defined field_ids
        count = count + $01
    end if
    if __type_list_count > $00
        count = count + $01
    end if
    if __code_item_count > $00
        count = count + $01
    end if
    count = count + $01

    align $04
    data_map:
    _map:
        uint count
        emit_map_item TYPE_HEADER_ITEM,      $01,             header
        emit_map_item TYPE_STRING_ID_ITEM,   string_ids_size, string_ids
        emit_map_item TYPE_TYPE_ID_ITEM,     type_ids_size,   type_ids
        emit_map_item TYPE_PROTO_ID_ITEM,    proto_ids_size,  proto_ids
        if defined field_ids
            emit_map_item TYPE_FIELD_ID_ITEM, field_ids_size, field_ids
        end if
        emit_map_item TYPE_METHOD_ID_ITEM,   method_ids_size, method_ids
        emit_map_item TYPE_CLASS_DEF_ITEM,   class_defs_size, class_defs
        emit_map_item TYPE_STRING_DATA_ITEM, string_ids_size, data_strings
        if __type_list_count > $00
            emit_map_item TYPE_TYPE_LIST,    __type_list_count, data_type_lists
        end if
        emit_map_item TYPE_CLASS_DATA_ITEM,  class_defs_size, data_class_data
        if __code_item_count > $00
            emit_map_item TYPE_CODE_ITEM,    __code_item_count, data_code
        end if
        emit_map_item TYPE_MAP_LIST,         $01,             data_map

    align $04
end macro
; ── Method code helper ───────────────────────────────────
;
;  Usage:
;      dex_method _hello_init_code, $01, $01, $01
;          invoke_direct _act_init_m, v0
;          return_void
;      end dex_method

macro dex_method? _name?*, _registers_size?*, _ins_size?*, _outs_size?*
    __dex_method_name equ _name
    __dex_method_regs = _registers_size
    __dex_method_ins = _ins_size
    __dex_method_outs = _outs_size
    ; Parameter register aliases. In Dalvik, method parameters live in
    ; the high registers: pN = v(registers - ins + N). Declaring them
    ; as '=' (not 'equ') makes the values resolve now, so they stay
    ; consistent even if someone later reassigns __dex_method_regs.
    p0  = _registers_size - _ins_size + 0
    p1  = _registers_size - _ins_size + 1
    p2  = _registers_size - _ins_size + 2
    p3  = _registers_size - _ins_size + 3
    p4  = _registers_size - _ins_size + 4
    p5  = _registers_size - _ins_size + 5
    p6  = _registers_size - _ins_size + 6
    p7  = _registers_size - _ins_size + 7
    p8  = _registers_size - _ins_size + 8
    p9  = _registers_size - _ins_size + 9
    p10 = _registers_size - _ins_size + 10
    p11 = _registers_size - _ins_size + 11
    p12 = _registers_size - _ins_size + 12
    p13 = _registers_size - _ins_size + 13
    p14 = _registers_size - _ins_size + 14
    p15 = _registers_size - _ins_size + 15
    virtual at $00
    _name#_insns::
end macro

macro end?.dex_method?
    end virtual
    match name, __dex_method_name
        name code_item __dex_method_regs, __dex_method_ins, __dex_method_outs, $00, $00, $FFFF, name#_insns
    end match
    restore __dex_method_name
    align $04
end macro
