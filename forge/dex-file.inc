; ══════════════════════════════════════════════════════════
;  dex-file.inc — the header, the map, and the order of sections
;
;  A dex file is a header, then sections in a fixed order, then a
;  map that says where each of them went and how many items it held.
;  The header cannot be written until the sizes are known and the
;  map cannot be written until the offsets are, so both are laid out
;  as labels and filled in by the assembler.
;
;  The map is built rather than declared: whichever sections turned
;  out to be non-empty are counted and listed.  Getting that count
;  wrong is the kind of mistake that produces a file the verifier
;  rejects with nothing useful to say, so it is derived, never
;  written.
; ══════════════════════════════════════════════════════════

; ── Header ───────────────────────────────────────────────
;
;  The first section, and the one that says where the others are.
;  A section with no items has no offset either: the format wants
;  zero in both.  Of the sections the header names, field_ids is
;  the only one that can be empty.

macro dex_header?
        emit $08 : DEX_FILE_MAGIC       ; 8 raw bytes, not one field
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
        uint data_map                       ; map_off
        uint string_ids_size
        uint (string_ids  - header)
        uint type_ids_size
        uint (type_ids    - header)
        uint proto_ids_size
        uint (proto_ids   - header)
        if field_ids_size > $00
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
        uint (link_data - data_strings)     ; data_size
        uint (data_strings - header)        ; data_off
end macro

macro dex_footer?
    link_data:
    header_end:

    emit_checksums
end macro

; ── The file ─────────────────────────────────────────────
;
;      dex_lists _class, _types, _methods, _code
;      dex_file
;
;  The layout is the same every time, so it is written once here.
;  The one thing that varies is the list of interfaces a class
;  implements, which goes in a macro handed to dex_file.

macro dex_file? interfaces?
    postpone
    __global::
        __dex_interfaces equ interfaces
        _dex_collect
        _dex_sections _dex_write
        dex_map_list
        dex_footer
    end postpone
end macro

; ── Sections ─────────────────────────────────────────────
;
;  Every section, in the order the file has them: what the map
;  calls it, the label it starts at, how many items it holds, and
;  what writes it.  The list is read twice — once to write the
;  sections, once to write the map that says where they went — so
;  the two cannot disagree about what is in the file.
;
;  Every section is written, an empty one as nothing at all, and
;  only those that hold something are listed in the map.  The map
;  itself names no writer: it reads this list, which a macro cannot
;  do from inside its own expansion, so dex_file writes it after.  The data
;  sections start on a four-byte boundary; the ones before them are
;  made of items whose sizes keep it.

macro _dex_sections? section?*
    section TYPE_HEADER_ITEM,      header,          $01,               dex_header
    section TYPE_STRING_ID_ITEM,   string_ids,      string_ids_size,   emit_string_ids
    section TYPE_TYPE_ID_ITEM,     type_ids,        type_ids_size,     emit_type_ids
    section TYPE_PROTO_ID_ITEM,    proto_ids,       proto_ids_size,    emit_proto_ids
    section TYPE_FIELD_ID_ITEM,    field_ids,       field_ids_size,    emit_field_ids
    section TYPE_METHOD_ID_ITEM,   method_ids,      method_ids_size,   emit_method_ids
    section TYPE_CLASS_DEF_ITEM,   class_defs,      class_defs_size,   emit_class_defs
    section TYPE_STRING_DATA_ITEM, data_strings,    string_ids_size,   emit_string_data
    section TYPE_TYPE_LIST,        data_type_lists, __type_list_count, _dex_type_lists
    section TYPE_CLASS_DATA_ITEM,  data_class_data, class_defs_size,   _dex_class_data
    section TYPE_CODE_ITEM,        data_code,       __code_item_count, emit_code
    section TYPE_MAP_LIST,         data_map,        $01
end macro

macro _dex_write? type?*, at?*, size?*, writer?
    align $04
  at:
    writer
end macro

macro _dex_map_item? type?*, at?*, size?*, writer?
    if size > $00
        ushort type
        ushort $00
        uint   size
        uint   at
    end if
end macro

;  The count is the distance between two labels over the size of
;  an item, so it cannot be wrong about which items were written.
;  Getting it wrong is the kind of mistake that produces a file the
;  verifier rejects with nothing useful to say.
macro dex_map_list?
        uint (data_map_end - data_map_items) / $0C
    data_map_items:
        _dex_sections _dex_map_item
    data_map_end:
    align $04
end macro

;  The prototypes' parameter lists, then the classes' interface
;  lists, which are type lists too and are counted with them.
macro _dex_type_lists?
    __tl_seq = $00
    emit_proto_type_lists
    align $04
    match i, __dex_interfaces
        i
    end match
    __tl_total = __tl_seq
end macro

macro _dex_class_data?
    repeat __class_count r:$00
        repeat 1 k:__cord_#r
            __class_data_#k defclassdata k
        end repeat
    end repeat
end macro
