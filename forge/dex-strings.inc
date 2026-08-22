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
;      data:
;          data_strings:
;              emit_string_data
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
; __emitting is deliberately not touched here: count_fields collects
; without it, and moving it in would change what defclass does.
macro _collect? pool?*, mode?*
    pool#_mode = mode
    virtual at $00
        _walk_lists
    end virtual
    pool#_mode = $00
end macro

__snum   = $00              ; distinct strings in the pool
__sphase = $00              ; 0 idle, 1 collect, 2 bind

; Interning reaches entries by a computed index, which macros can
; only do by turning the value into a repeat counter first, since #
; joins tokens rather than values.  A CALM instruction builds the
; name with arrange and then reads it with compute or writes it
; with publish, which is what those are for.
;
; The scan stops at the first match rather than running to the end.

calminstruction _intern? value*, len*
    local e, n, sym, v, found, val, ln
    ; Evaluate the arguments once.  publish keeps what it is given
    ; rather than what that meant at the time, so handing it a
    ; variable the caller goes on to reassign — the shorty being
    ; built for each prototype in turn — would leave every entry
    ; tracking the last one.
    compute val, value
    compute ln, len
    compute n, __snum
    compute e, 0
    compute found, -1
  scan:
    check e < n
    jno add
    arrange sym, =__strl_#e
    compute v, sym
    check v = ln
    jno step
    arrange sym, =__strv_#e
    compute v, sym
    check v = val
    jno step
    compute found, e
    jump done
  step:
    compute e, e + 1
    jump scan
  add:
    arrange sym, =__strv_#n
    publish sym, val
    arrange sym, =__strl_#n
    publish sym, ln
    compute found, n
    compute n, n + 1
    arrange sym, =__snum
    publish sym, n
  done:
    arrange sym, =__sfound
    publish sym, found
end calminstruction

; Sorting is by UTF-16 code point value, which is not MUTF-8 byte
; order: a supplementary character is encoded as a surrogate pair
; starting with $ED, so byte order puts it below U+FFFD while code
; point order puts it above.  Decode and compare code points.
;
; This is the hottest thing in the file — the sort calls it once per
; step of every binary search — so it is a CALM instruction rather
; than a macro.  Measured over 2000 comparisons of two 43-character
; strings differing only at the end: 2.20s as a macro, 0.15s here.
;
; Note that a label may not be called store, which is a CALM
; instruction of its own.

calminstruction _str_less? result*, va*, la*, vb*, lb*
    local ia, ib, ca, cb, b, b1, b2, b3, r
    compute ia, 0
    compute ib, 0
    compute r, 0
  loop:
    check ia < la
    jno tail
    check ib < lb
    jno tail
    ; decode the code point of va at ia
    compute b, (va shr (ia * 8)) and $FF
    check b < $80
    jyes a1
    check b < $E0
    jyes a2
    check b < $F0
    jyes a3
    compute b1, (va shr ((ia + 1) * 8)) and $3F
    compute b2, (va shr ((ia + 2) * 8)) and $3F
    compute b3, (va shr ((ia + 3) * 8)) and $3F
    compute ca, ((b and $07) shl 18) or (b1 shl 12) or (b2 shl 6) or b3
    compute ia, ia + 4
    jump adone
  a1:
    compute ca, b
    compute ia, ia + 1
    jump adone
  a2:
    compute b1, (va shr ((ia + 1) * 8)) and $3F
    compute ca, ((b and $1F) shl 6) or b1
    compute ia, ia + 2
    jump adone
  a3:
    compute b1, (va shr ((ia + 1) * 8)) and $3F
    compute b2, (va shr ((ia + 2) * 8)) and $3F
    compute ca, ((b and $0F) shl 12) or (b1 shl 6) or b2
    compute ia, ia + 3
  adone:
    ; and of vb at ib
    compute b, (vb shr (ib * 8)) and $FF
    check b < $80
    jyes c1
    check b < $E0
    jyes c2
    check b < $F0
    jyes c3
    compute b1, (vb shr ((ib + 1) * 8)) and $3F
    compute b2, (vb shr ((ib + 2) * 8)) and $3F
    compute b3, (vb shr ((ib + 3) * 8)) and $3F
    compute cb, ((b and $07) shl 18) or (b1 shl 12) or (b2 shl 6) or b3
    compute ib, ib + 4
    jump cdone
  c1:
    compute cb, b
    compute ib, ib + 1
    jump cdone
  c2:
    compute b1, (vb shr ((ib + 1) * 8)) and $3F
    compute cb, ((b and $1F) shl 6) or b1
    compute ib, ib + 2
    jump cdone
  c3:
    compute b1, (vb shr ((ib + 1) * 8)) and $3F
    compute b2, (vb shr ((ib + 2) * 8)) and $3F
    compute cb, ((b and $0F) shl 12) or (b1 shl 6) or b2
    compute ib, ib + 3
  cdone:
    check ca = cb
    jyes loop
    check ca < cb
    jno fin
    compute r, 1
    jump fin
    ; every code point compared equal, so the shorter one is first
  tail:
    check la < lb
    jno fin
    compute r, 1
  fin:
    publish result, r
end calminstruction

; Binary insertion sort, leaving the pool index of position r in
; __ord_#r.  Insertion keeps the expensive part, comparing two
; strings, down to a binary search per entry, and pays for it in
; plain integer moves.
;
; fasmg's # joins tokens rather than values, so reaching an entry
; through a computed index means turning that value into a repeat
; counter first, hence the nested repeat 1.

macro _sort_strings? n?*
    __sort_m = $00
    repeat n i:$00
        __sort_lo = $00
        __sort_hi = __sort_m
        while __sort_lo < __sort_hi
            __sort_mid = (__sort_lo + __sort_hi) / $02
            repeat 1 mid:__sort_mid
                repeat 1 k:__ord_#mid
                    _str_less __sort_lt, __strv_#k, __strl_#k, __strv_#i, __strl_#i
                end repeat
            end repeat
            if __sort_lt
                __sort_lo = __sort_mid + $01
            else
                __sort_hi = __sort_mid
            end if
        end while
        __sort_j = __sort_m
        while __sort_j > __sort_lo
            repeat 1 a:__sort_j
                repeat 1 b:(a - $01)
                    __ord_#a = __ord_#b
                end repeat
            end repeat
            __sort_j = __sort_j - $01
        end while
        repeat 1 a:__sort_lo
            __ord_#a = i
        end repeat
        __sort_m = __sort_m + $01
    end repeat
    ; The comparator decided the order, so re-reading it proves
    ; nothing about itself; this catches a slip in the insertion.
    ; Interning means equal strings never reach here.
    if n > $01
        repeat n - $01 r:$00, r1:$01
            repeat 1 a:__ord_#r
                repeat 1 b:__ord_#r1
                    _str_less __sort_lt, __strv_#a, __strl_#a, __strv_#b, __strl_#b
                    if ~ __sort_lt
                        err 'string pool came out unsorted'
                    end if
                end repeat
            end repeat
        end repeat
    end if
end macro

; Each declaration that contributes a string remembers which pool
; entry it landed in, so the binding pass can turn that into the
; position the sort gave it.

macro defstr? base?*, text?*&
    if __sphase = $01
        _intern text, lengthof text
        repeat 1 k:__defstr_index
            __dstr_slot_#k = __sfound
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
    __snum = $00
    __sphase = $01
    __defstr_index = $00
    __defproto_index = $00
    ; The pool lists emit nothing in this phase, but the code does,
    ; so everything goes into a block that is thrown away.
    virtual at $00
        _walk_lists
    end virtual

    _sort_strings __snum
    repeat __snum r:$00
        repeat 1 e:__ord_#r
            __srank_#e = r
        end repeat
    end repeat

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

    repeat __snum r:$00
        repeat 1 e:__ord_#r
            __sid_#e string_id_item __sdata_#e
        end repeat
    end repeat
end macro

; The pool is already built, so this needs no list.
macro emit_string_data?
    repeat __snum r:$00
        repeat 1 e:__ord_#r
            __sdata_#e defstring __strv_#e, __strl_#e
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
