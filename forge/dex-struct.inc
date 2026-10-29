; ══════════════════════════════════════════════════════════
;  dex-struct.inc — the items a dex file is made of
;
;  One struc per item the format defines, in the order the file
;  lays them out: the five id pools, the class definitions, the
;  lists they point at, and the code.
;
;  Every field goes through a width-checked emitter from base.inc
;  rather than through db/dw/dd, because fasmg widens a too-large
;  value instead of truncating it — and a field that grew by two
;  bytes shifts every offset after it in the file.
;
;  __define_index / __update_index track where each item landed, so
;  the header and the map can say how many there are and where they
;  begin without anyone counting.
; ══════════════════════════════════════════════════════════

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
    _order_strict __order_type_id, _descriptor_idx, 'type_ids'
    align $04
    .descriptor_idx?: uint _descriptor_idx
end struc

__define_index __proto_id_item?

struc proto_id_item? _shorty_idx?*, _return_type_idx?*, _parameters_off?*
    __update_index ., __proto_id_item?
    ; parameter lists live in the data section and are not readable
    ; from here, so only the return type is checked
    _order_weak __order_proto_id, _return_type_idx, 'proto_ids'
    align $04
    .shorty_idx?:      uint _shorty_idx
    .return_type_idx?: uint _return_type_idx
    .parameters_off?:  uint _parameters_off
end struc

__define_index __field_id_item?

struc field_id_item? _class_idx?*, _type_idx?*, _name_idx?*
    __update_index ., __field_id_item?
    _order_strict __order_field_id, \
        ((_class_idx) shl $40) or ((_name_idx) shl $20) or (_type_idx), 'field_ids'
    align $04
    .class_idx?: ushort _class_idx
    .type_idx?:  ushort _type_idx
    .name_idx?:  uint   _name_idx
end struc

__define_index __method_id_item?

struc method_id_item? _class_idx?*, _proto_idx?*, _name_idx?*
    __update_index ., __method_id_item?
    _order_strict __order_method_id, \
        ((_class_idx) shl $40) or ((_name_idx) shl $20) or (_proto_idx), 'method_ids'
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

__type_list_count? = 0
__tl_seq = 0

; The body of a type list, written from where it is to start, which
; is four-byte aligned: the count, then each type in turn.  Every
; list is also kept as a row of a table — where it is, how many
; types, which — for emit_class_defs, which has to know the
; interfaces a class lists before they are written.  A row is set
; once a pass, so the earlier reader gets the last pass's.
macro _type_list? n?*
    __type_list_count? = __type_list_count? + 1
    __tl_cur = __tl_seq
    __tl_j = 0
    repeat 1 i:__tl_cur
        __tl_at_#i = $
        __tl_n_#i = n
    end repeat
    __tl_seq = __tl_seq + 1
    uint n
end macro

macro _type_list_type? t?*
    repeat 1 i:__tl_cur, j:__tl_j
        __tl_t_#i#_#j = t
    end repeat
    __tl_j = __tl_j + 1
    ushort t
end macro

; The four list sizes are the lengths of the four lists, so count
; them rather than have the caller restate them.

macro _count_list? result?*, list?&
    result = $00
    iterate item?, list
        result = result + $01
    end iterate
end macro

struc class_data_item? _static_fields?, _instance_fields?, _direct_methods?, _virtual_methods?
    local ns, ni, nd, nv
    _count_list ns, _static_fields
    _count_list ni, _instance_fields
    _count_list nd, _direct_methods
    _count_list nv, _virtual_methods
    label .
    .static_fields_size?:   leb128 ns
    .instance_fields_size?: leb128 ni
    .direct_methods_size?:  leb128 nd
    .virtual_methods_size?: leb128 nv
    _enc_group_start
    .static_fields?:
    iterate static_field?, _static_fields
        .static_fields?.% encoded_field static_field
    end iterate
    _enc_group_start
    .instance_fields?:
    iterate instance_field?, _instance_fields
        .instance_fields?.% encoded_field instance_field
    end iterate
    _enc_group_start
    .direct_methods?:
    iterate direct_method?, _direct_methods
        .direct_methods?.% encoded_method direct_method
    end iterate
    _enc_group_start
    .virtual_methods?:
    iterate virtual_method?, _virtual_methods
        .virtual_methods?.% encoded_method virtual_method
    end iterate
end struc

; Encoded field and method lists store an index as a difference
; from the previous entry, the first entry being absolute.  Callers
; name the field or method they mean and the difference is computed
; here, so inserting an entry cannot shift the ones after it.
;
; Each of the four lists restarts the numbering, hence the group
; reset.  Differences must be positive: the format requires each
; list to be sorted by ascending index.

__enc_first = $01
__enc_prev  = $00

macro _enc_group_start?
    __enc_first = $01
    __enc_prev  = $00
end macro

macro _enc_idx_diff? result?*, idx?*, what?*
    if __enc_first
        result = idx
        __enc_first = $00
    else
        result = (idx) - __enc_prev
        if result <= $00
            err what, ' must be listed in ascending index order'
        end if
    end if
    __enc_prev = idx
end macro

struc encoded_field? _field_idx?*, _access_flags?*
    local diff
    _enc_idx_diff diff, _field_idx, 'fields'
    .field_idx_diff?: leb128 diff
    .access_flags?:   leb128 _access_flags
end struc

struc encoded_method? _method_idx?*, _access_flags?*, _code_off?*
    local diff
    _enc_idx_diff diff, _method_idx, 'methods'
    .method_idx_diff?: leb128 diff
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
        err 'try/catch is not supported yet, pass tries_size = 0'
    end if
end struc
