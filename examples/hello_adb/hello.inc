; https://board.flatassembler.net/topic.php?t=21689

format binary as "dex"
org $00

macro _assert_unsigned? list?*&
    iterate item?*, list
        assert (item >= $00)
    end iterate
end macro

macro byte? list?*&
    db list
end macro

macro ubyte? list?*&
    _assert_unsigned list
    db list
end macro

macro short? list?*&
    db list
end macro

macro ushort? list?*&
    _assert_unsigned list
        dw list
end macro

macro int? list?*&
        dd list
end macro

macro uint? list?*&
    _assert_unsigned list
        dd list
end macro

macro long? list?*&
    dq list
end macro

macro ulong? list?*&
    _assert_unsigned list
    dq list
end macro

macro leb128? list?*& ; uleb128 & sleb128
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

DEX_FILE_MAGIC? := $6465780A30333500 bswap $08
ENDIAN_CONSTANT? := $12345678
REVERSE_ENDIAN_CONSTANT? := $78563412
NO_INDEX? := $FFFFFFFF

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
        .shorty_idx?: uint _shorty_idx
        .return_type_idx?: uint _return_type_idx
        .parameters_off?: uint _parameters_off
end struc

__define_index __field_id_item?

struc field_id_item? _class_idx?*, _type_idx?*, _name_idx?*
    __update_index ., __field_id_item?
    align $04
        .class_idx?: ushort _class_idx
        .type_idx?: ushort _type_idx
        .name_idx?: uint _name_idx
end struc

__define_index __method_id_item?

struc method_id_item? _class_idx?*, _proto_idx?*, _name_idx?*
    __update_index ., __method_id_item?
    align $04
    .class_idx?: ushort _class_idx
        .proto_idx?: ushort _proto_idx
        .name_idx?: uint _name_idx
end struc

ACC_PUBLIC?             := $000001
ACC_PRIVATE?            := $000002
ACC_PROTECTED?          := $000004
ACC_STATIC?             := $000008
ACC_FINAL?              := $000010
ACC_NATIVE?             := $000100
ACC_INTERFACE?          := $000200
ACC_ABSTRACT?           := $000400
ACC_ANNOTATION?         := $002000
ACC_ENUM?               := $004000
ACC_CONSTRUCTOR?        := $010000

__define_index __class_def_item?

struc class_def_item?           _class_idx?*, _access_flags?*, _superclass_idx?*, _interfaces_off?*,\
                                _source_file_idx?*, _annotations_off?*, _class_data_off?*, _static_values_off?*
    __update_index ., __class_def_item?
    align $04
    .class_idx?: uint _class_idx
    .access_flags?: uint _access_flags
    .superclass_idx?: uint _superclass_idx
    .interfaces_off?: uint _interfaces_off
    .source_file_idx?: uint _source_file_idx
    .annotations_off?: uint _annotations_off
    .class_data_off?: uint _class_data_off
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

TYPE_HEADER_ITEM?           := $0000
TYPE_STRING_ID_ITEM?        := $0001
TYPE_TYPE_ID_ITEM?          := $0002
TYPE_PROTO_ID_ITEM?         := $0003
TYPE_FIELD_ID_ITEM?         := $0004
TYPE_METHOD_ID_ITEM?        := $0005
TYPE_CLASS_DEF_ITEM?        := $0006
TYPE_CALL_SITE_ID_ITEM?     := $0007
TYPE_METHOD_HANDLE_ITEM?    := $0008
TYPE_MAP_LIST?              := $1000
TYPE_TYPE_LIST?             := $1001
TYPE_CLASS_DATA_ITEM?       := $2000
TYPE_CODE_ITEM?             := $2001
TYPE_STRING_DATA_ITEM?      := $2002

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
    .type?: ushort _type
    ushort $00
    .size?: uint _size
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
    .static_fields_size?: leb128 _static_fields_size
    .instance_fields_size?: leb128 _instance_fields_size
    .direct_methods_size?: leb128 _direct_methods_size
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

struc encoded_field? _field_idx_diff?*, access_flags?*
    .field_idx_diff?: leb128 _field_idx_diff
    .access_flags?: leb128 _access_flags
end struc

struc encoded_method? _method_idx_diff?*, _access_flags?*, _code_off?*
    .method_idx_diff?: leb128 _method_idx_diff
    .access_flags?: leb128 _access_flags
    .code_off?: leb128 _code_off
end struc

struc code_item? _registers_size?*, _ins_size?*, _outs_size?*, _tries_size?*, _debug_info_off?*, _insns_size?*, _insns?*
    local base?, size?, opcode?
    align $04
    label .
    .registers_size?: ushort _registers_size
    .ins_size?: ushort _ins_size
    .outs_size?: ushort _outs_size
    .tries_size?: ushort _tries_size
    .debug_info_off?: uint _debug_info_off
    virtual _insns
        base? = $$
        size? =  $ - $$
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
    .start_addr?: uint _start_addr
    .insn_count?: ushort _insn_count
    .handler_off?: ushort _handler_off
end struc

postpone
__global::

    header:
        ubyte DEX_FILE_MAGIC
    header_magic_end:
        uint $00
    header_adler32:
        emit $14: $00
    header_sha1:
        uint (header_end - header) ; file_size
        uint (string_ids - header) ; header_size
        uint ENDIAN_CONSTANT ; endian_tag
        uint $00 ; link_size
        uint $00 ; link_off
        uint _map ; map_off
        uint string_ids_size
        uint (string_ids - header) ; string_ids_off
        uint type_ids_size
        uint (type_ids - header) ; type_ids_off
        uint proto_ids_size
        uint (proto_ids - header) ; proto_ids_off
        uint field_ids_size
        uint (field_ids - header) ; field_ids_off
        uint method_ids_size
        uint (method_ids - header) ; method_ids_off
        uint class_defs_size
        uint (class_defs - header) ; class_defs_off
        uint (link_data - data) ; data_size
        uint (data - header) ; data_off

    string_ids:
        _hello_string string_id_item _hello_data
        _class_hello_name_string string_id_item _class_hello_name_data
        _java_lang_print_stream_string string_id_item _java_lang_print_stream_data
        _java_lang_object_string string_id_item _java_lang_object_data
        _string_string string_id_item _string_data
        _java_lang_system_string string_id_item _java_lang_system_data
        _void_string string_id_item _void_data
        _class_hello_main_shorty_string string_id_item _class_hello_main_shorty_data
        _main_argument_1_string string_id_item _main_argument_1_data
        _main_string string_id_item _main_data
        _out_string string_id_item _out_data
        _println_string string_id_item _println_data

    type_ids:
        _class_hello_type type_id_item _class_hello_name_string
        _java_lang_print_stream_type type_id_item _java_lang_print_stream_string
        _java_lang_object_type type_id_item _java_lang_object_string
        _string_type type_id_item _string_string
        _java_lang_system_type type_id_item _java_lang_system_string
        _void_type type_id_item _void_string
        _array_string_type type_id_item _main_argument_1_string

    proto_ids:
        _println_proto proto_id_item _class_hello_main_shorty_string, _void_type, _println_type_list
        _main_proto proto_id_item _class_hello_main_shorty_string, _void_type, _main_type_list

    field_ids:
        _out_system_field field_id_item _java_lang_system_type, _java_lang_print_stream_type, _out_string

    method_ids:
        _class_hello_main_method method_id_item _class_hello_type, _main_proto, _main_string
        _println_method method_id_item _java_lang_print_stream_type, _println_proto, _println_string

    class_defs:
        _class_hello_class class_def_item _class_hello_type, ACC_PUBLIC, _java_lang_object_type, $00, NO_INDEX, $00, _class_hello_data, $00

    call_site_ids:
    method_handles:

    data:
        data_string_data_item:
            _hello_data string_data_item                    $05, "HELLO"
            _class_hello_name_data string_data_item         $0C, "LHelloWorld;"
            _java_lang_print_stream_data string_data_item   $15, "Ljava/io/PrintStream;"
            _java_lang_object_data string_data_item         $12, "Ljava/lang/Object;"
            _string_data string_data_item                   $12, "Ljava/lang/String;"
            _java_lang_system_data string_data_item         $12, "Ljava/lang/System;"
            _void_data string_data_item                     $01, "V"
            _class_hello_main_shorty_data string_data_item  $02, "VL"
            _main_argument_1_data string_data_item          $13, "[Ljava/lang/String;"
            _main_data string_data_item                     $04, "main"
            _out_data string_data_item                      $03, "out"
            _println_data string_data_item                  $07, "println"

        align $04

        data_type_list:
            _main_type_list type_list $01, <_array_string_type>
            _println_type_list type_list $01, <_string_type>

        align $04

        data_class_data_item:
            _class_hello_data class_data_item $00, $00, $01, $00,\
                <>,\
                <>,\
                <<_class_hello_main_method, ACC_PUBLIC or ACC_STATIC, _class_hello_code_data>>,\
                <>

        align $04

        data_code_item:
            _class_hello_code_data code_item $02, $01, $02, $00, $00, $08, _class_hello_code

        align $04

        data_map_list:
            _map map_list $0C, <\
                    <TYPE_HEADER_ITEM, $01, header>,\
                    <TYPE_STRING_ID_ITEM, string_ids_size, string_ids>,\
                    <TYPE_TYPE_ID_ITEM, type_ids_size, type_ids>,\
                    <TYPE_PROTO_ID_ITEM, proto_ids_size, proto_ids>,\
                    <TYPE_FIELD_ID_ITEM, field_ids_size, field_ids>,\
                    <TYPE_METHOD_ID_ITEM, method_ids_size, method_ids>,\
                    <TYPE_CLASS_DEF_ITEM, class_defs_size, class_defs>,\
                    <TYPE_STRING_DATA_ITEM, $0C, data_string_data_item>,\
                    <TYPE_TYPE_LIST, $02, data_type_list>,\
                    <TYPE_CLASS_DATA_ITEM, $01, data_class_data_item>,\
                    <TYPE_CODE_ITEM, $01, data_code_item>,\
                    <TYPE_MAP_LIST, $01, data_map_list>,\
                >

        align $04
        
    link_data:

    header_end:

        __snapshot_index string_ids_size, __string_id_item
        __snapshot_index type_ids_size, __type_id_item
        __snapshot_index proto_ids_size, __proto_id_item
        __snapshot_index field_ids_size, __field_id_item        
        __snapshot_index method_ids_size, __method_id_item
        __snapshot_index class_defs_size, __class_def_item

        assert (type_ids_size <= $FFFF)

    load payload : ($ - header_sha1) from __global : header_sha1
    sha1_payload sha1 payload

    store sha1_payload : $14 at __global : header_adler32

    load payload : ($ - header_adler32) from __global : header_adler32
    adler32_payload adler32 payload
 
    store adler32_payload : $04 at __global : header_magic_end
   
end postpone

virtual at $00
_class_hello_code::
    dw $0062, _out_system_field         ; sget-object v0, _out_system_field
    dw $011A, _hello_string             ; const-string v1, _hello_string
    dw $206E, _println_method, $0010    ; invoke-virtual {v0,v1}, _println_method
    dw $000E                            ; return-void
end virtual
