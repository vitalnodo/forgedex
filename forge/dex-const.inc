; ══════════════════════════════════════════════════════════
;  dex-const.inc — what the format fixes, and where things landed
;
;  Three unrelated things that are all bookkeeping rather than
;  output:
;
;    • the index tracking macros, which remember where each kind of
;      item began and how many there were, so the header and the map
;      can say so without anyone counting;
;    • the constants the format defines — magic, endianness, access
;      flags, the map section types;
;    • _order_strict / _order_weak, which fail the assembly if a pool
;      comes out unsorted.  The format requires the order; these are
;      what makes a mistake in it loud.
; ══════════════════════════════════════════════════════════

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

; ── Pool ordering ─────────────────────────────────────────
;
;  The format requires type_ids, proto_ids, field_ids, method_ids
;  and string_ids to be sorted, and a violation is not caught here
;  but by the verifier on the device, long after the fact.
;
;  Composite keys are packed into one integer so a single numeric
;  comparison covers all their components, major part first.
;
;  Sort keys, from the format:
;    type_ids    descriptor_idx
;    field_ids   class_idx, then name_idx, then type_idx
;    method_ids  class_idx, then name_idx, then proto_idx
;    proto_ids   return_type_idx, then the parameter list compared
;                element by element
;
;  Every pass re-runs this file, so plain assignment is enough to
;  reset the running keys between passes.

__order_type_id   = -$01
__order_proto_id  = -$01
__order_field_id  = -$01
__order_method_id = -$01

; Entries must strictly increase: equal keys would be duplicates.
macro _order_strict? state?*, key?*, what?*
    if (key) <= state
        err what, ' are out of order or duplicated'
    end if
    state = key
end macro

; Only a prefix of the key is known here, so equal keys are allowed
; and merely leave the rest of the ordering unchecked.
macro _order_weak? state?*, key?*, what?*
    if (key) < state
        err what, ' are out of order'
    end if
    state = key
end macro
