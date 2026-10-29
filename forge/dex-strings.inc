; ══════════════════════════════════════════════════════════
;  dex-strings.inc — the string pool, and the walk that fills it
;
;  Two things that belong together because one exists to serve the
;  other.
;
;  The walk: the declarations are read once, by _dex_collect in
;  dex-pools.inc, and everything they name goes into its pool on
;  the way.  The code is assembled in the same walk, each method body
;  into an area of its own, with the ranks the pools settle to later
;  in the pass; the walk itself is inside a virtual block, since the
;  lists write nothing where they stand.
;
;  The pool: strings are interned rather than declared — asking for
;  one that is already there gives back the one that is there.  The
;  sort is by UTF-16 code unit, which is what ART checks; _str_less
;  in base.inc says how.
; ══════════════════════════════════════════════════════════

; ── String pool ──────────────────────────────────────────
;
;      macro _strings
;          defstr _init, "<init>"
;      end macro
;
;      macro _strings
;          defstr _init, "<init>"
;      end macro
;
;  Strings come from everywhere: the literals declared here and in
;  the code, the descriptors of every type named, member names, and
;  the shorties worked out from the prototypes.  A defstr also gives
;  its string a name, _init_string, which holds the position the
;  sort settled.
;
;  Interning, not declaring: asking for a string that is already in
;  the pool gives back the one already there.  A type descriptor
;  and a shorty may well be the same text, "V" being both, and
;  neither has to know about the other.
;
;  A string is held as its bytes packed into one value plus a byte
;  count, so two of them are equal when both parts match.

; Clear while the lists are collected, so that a type list met
; then contributes its types instead of writing itself.
__emitting = $01

__defclass_index = $00
__class_count = $00


; The lists a file is built from, named once.  Both walks read all
; of them, so there is no per-pool list to keep in step.
macro dex_lists? lists?*&
    define __dex_lists lists
end macro

macro _walk_lists?
    __defclass_index = $00
    match L, __dex_lists
        iterate cb?, L
            cb
        end iterate
    end match
end macro

__str_n  = $00              ; distinct strings in the pool

; Each declaration that contributes a string remembers which pool
; entry it landed in, and under what name, so that _dex_settle can
; give the name the position the sort gave the entry.

macro defstr? base?*, text?*&
    _intern __str, add, text, lengthof text
    repeat 1 k:__defstr_index
        __dstr_slot_#k = __str_found
        __dstr_name_#k equ base
    end repeat
    __defstr_index = __defstr_index + $01
end macro

macro emit_string_ids?
    repeat __str_n r:$00
        repeat 1 e:__ord_#r
            __sid_#e string_id_item __sdata_#e
        end repeat
    end repeat
end macro

; The pool is already built, so this needs no list.
macro emit_string_data?
    repeat __str_n r:$00
        repeat 1 e:__ord_#r
            __sdata_#e defstring __str_v_#e, __str_l_#e
        end repeat
    end repeat
end macro

; A member written "->name..." is relative to the class most
; recently declared, so a class descriptor is spelled once.
macro _ref_pair? ref?*&
    if ((ref) and $FF) = $2D
        __rf_v = __cls_v or ((ref) shl (__cls_l * $08))
        __rf_l = __cls_l + lengthof ref
    else
        __rf_v = ref
        __rf_l = lengthof ref
    end if
end macro
