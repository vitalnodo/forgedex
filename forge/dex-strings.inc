; ══════════════════════════════════════════════════════════
;  dex-strings.inc — the string pool, and the walk that fills it
;
;  Two things that belong together because one exists to serve the
;  other.
;
;  The walk: every pool is built by reading the declarations several
;  times over — once to learn what strings exist, once for types,
;  once for prototypes, and finally to emit.  The lists emit nothing
;  during a collecting pass, but the instructions among them emit
;  whatever they are asked to, so the walk happens inside a virtual
;  block and the bytes are thrown away.
;
;  The pool: strings are interned rather than declared — asking for
;  one that is already there gives back the one that is there.  The
;  sort is by UTF-16 code point, *not* by MUTF-8 byte: a surrogate
;  pair for U+10000 and above begins with $ED, so comparing bytes
;  would put it in the wrong place.
;
;  _str_less is the hottest thing in the whole assembler — it runs
;  once per comparison of a binary insertion sort over every string
;  in the file — which is why it is CALM rather than macros.
; ══════════════════════════════════════════════════════════

; ── String pool ──────────────────────────────────────────
;
;      macro _strings
;          defstr _init, "<init>"
;      end macro
;
;      string_ids:
;          emit_string_ids _strings, _types, _protos
;      data_strings:
;          emit_string_data
;
;  Strings come from three places: the literals declared here, the
;  descriptors carried by the types, and the shorties worked out
;  from the prototypes.  All three lists are handed to
;  emit_string_ids so the pool can be built from every one of them,
;  and every list is expanded twice, once to collect and once to
;  bind the names to the positions the sort settled.
;
;  Interning, not declaring: asking for a string that is already in
;  the pool gives back the one already there.  A type descriptor
;  and a shorty may well be the same text, "V" being both, and
;  neither has to know about the other.
;
;  A string is held as its bytes packed into one value plus a byte
;  count, so two of them are equal when both parts match.

; Set while the output is really being produced.  Every pool walks
; the same lists to collect from them, and an instruction met during
; one of those walks contributes what it names instead of resolving
; it, because the pool it would resolve against is what is being
; built.
__emitting = $01

; Set only while the class_defs section is being produced, so
; that defclass does not emit its item during another pool's own
; emission, which walks the same lists.
__class_defs_phase = $00
__defclass_index = $00
__class_count = $00

; Set only while the code section is being produced.
__code_phase = $00

; The lists a file is built from, named once.  Every pool walks all
; of them and takes what concerns it, so there is no per-pool list
; to keep in step.
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

; Walk every list with one pool put into one of its collecting modes.
; The lists are being read for what they contribute, not for what they
; emit — but the instructions among them emit whatever they are asked
; to, so the walk happens inside a virtual block and the bytes are
; dropped.  Every pool does this at least once, some of them twice.
;
; __emitting is deliberately not touched here: every caller sets it
; first, and moving it in would change what defclass does.
macro _collect? pool?*, mode?*
    pool#_mode = mode
    virtual at $00
        _walk_lists
    end virtual
    pool#_mode = $00
end macro

__str_n  = $00              ; distinct strings in the pool
__sphase = $00              ; 0 idle, 1 collect, 2 bind

; Each declaration that contributes a string remembers which pool
; entry it landed in, so the binding pass can turn that into the
; position the sort gave it.

macro defstr? base?*, text?*&
    if __sphase = $01
        _intern __str, add, text, lengthof text
        repeat 1 k:__defstr_index
            __dstr_slot_#k = __str_found
        end repeat
        __defstr_index = __defstr_index + $01
    else if __sphase = $02
        repeat 1 k:__defstr_index
            repeat 1 e:__dstr_slot_#k
                base#_string = __srank_#e
            end repeat
        end repeat
        __defstr_index = __defstr_index + $01
    end if
end macro

macro emit_string_ids?
    __emitting = $00
    __str_n = $00
    __sphase = $01
    __defstr_index = $00
    __defproto_index = $00
    ; The pool lists emit nothing in this phase, but the code does,
    ; so everything goes into a block that is thrown away.
    virtual at $00
        _walk_lists
    end virtual

    ;  Interning means equal strings never reach the sort, so the
    ;  check it ends with can only catch a slip in the insertion.
    _order __str, _str_less, __srank_, __ord_, __str_n, \
        'string pool came out unsorted'

    ; The binding pass has one job: turn the pool slot a defstr landed
    ; in into the position the sort gave it, under the name that defstr
    ; was declared with.  A file that declares no strings by name has
    ; nothing for it to bind — and none of the examples do, since the
    ; pool fills itself from the code — so the walk is skipped rather
    ; than made over every list and every method body for nothing.
    if __defstr_index > $00
        __sphase = $02
        __defstr_index = $00
        virtual at $00
            _walk_lists
        end virtual
    end if
    __sphase = $00
    __emitting = $01

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
