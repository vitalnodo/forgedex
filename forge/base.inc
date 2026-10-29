; ══════════════════════════════════════════════════════════
;  base.inc — what every forged format needs underneath
;
;  A dex, a resources.arsc and a binary AXML have almost nothing
;  in common, but all three are laid out by writing fields of a
;  fixed width and aligning to a boundary, and all three keep
;  pools of values that are each written once and put in order,
;  strings among them that count their length in UTF-16 units.
;  That is all this is.
;
;  It is included by forgedex.inc, forgeres.inc and forgeaxml.inc
;  rather than by anything an example writes.  Before it existed,
;  the two smaller formats took these macros from forgedex.inc
;  without saying so, and a manifest of forty lines had to include
;  three files in one particular order or fail with a message
;  about a symbol it had never heard of.
;
;  Included more than once, the guard below makes the second time
;  a no-op.  `definite` rather than `defined`: the latter is
;  resolved late enough that the symbol appears mid-pass, and
;  fasmg then oscillates until it runs out of passes.
; ══════════════════════════════════════════════════════════

if ~ definite __forge_base_included
__forge_base_included := $01

; ── Assertion helpers ─────────────────────────────────────
;
;  _chk_u / _chk_s check that a value fits a field of the given
;  width.  Both are also used by the instruction format macros.

macro _assert_unsigned? list?*&
    iterate item?*, list
        assert (item >= $00)
    end iterate
end macro

macro _chk_u? value?*, bits?*, what?*
    if ~ ((value) >= $00 & (value) <= (($01 shl (bits)) - $01))
        err what, ' out of range'
    end if
end macro

macro _chk_s? value?*, bits?*, what?*
    if ~ ((value) >= -($01 shl ((bits) - $01)) & (value) <= (($01 shl ((bits) - $01)) - $01))
        err what, ' out of range'
    end if
end macro

; ── Primitive type macros ─────────────────────────────────
;
;  These emit one field of exactly the named width.  The check
;  matters because fasmg's db/dw/dd/dq widen instead of
;  truncating: a value too large for the field silently emits
;  extra units and shifts everything after it in the file.
;
;  To emit a value as a fixed-size blob regardless of its
;  magnitude, use fasmg's `emit size : value` directly.

macro _def_signed? name?*, bits?*, store?*
    macro name? list?*&
        iterate item?*, list
            _chk_s item, bits, `name
        end iterate
        store list
    end macro
end macro

macro _def_unsigned? name?*, bits?*, store?*
    macro name? list?*&
        iterate item?*, list
            _chk_u item, bits, `name
        end iterate
        store list
    end macro
end macro

_def_signed   byte,   $08, db
_def_unsigned ubyte,  $08, db
_def_signed   short,  $10, dw
_def_unsigned ushort, $10, dw
_def_signed   int,    $20, dd
_def_unsigned uint,   $20, dd
_def_signed   long,   $40, dq
_def_unsigned ulong,  $40, dq

; ── Interning ─────────────────────────────────────────────
;
;  Every pool is a list of values under one prefix: pool_n of
;  them, pool_v_0 onwards, with pool_l_#i beside each when the
;  pool keeps lengths and pool_i_#i when it keeps a payload.  This
;  finds a value and answers its index in pool_found.
;
;      miss      add: append it, and answer where it went
;                find: answer -1 and leave the pool as it is
;      len       compared as well as the value.  fasmg compares
;                strings as numbers, so "a" and "a",0 are the same
;                value, and only the length tells them apart.
;      payload   stored beside a value that is added
;
;  An entry is reached by a computed index, which macros can only
;  do by turning the value into a repeat counter first, since #
;  joins tokens rather than values.  A CALM instruction builds the
;  name with arrange and then reads it with compute or writes it
;  with publish, which is what those are for.  `pool#=_n` joins
;  the prefix it was given to a tail of its own; without the = the
;  tail would be read as a name local to this instruction.
;
;  The scan stops at the first match rather than running to the
;  end.

calminstruction _intern? pool*, miss*, value*, len, payload
    local e, n, sym, v, val, ln, found
    ;  Evaluate the arguments once.  publish keeps what it is given
    ;  rather than what that meant at the time, so handing it a
    ;  variable the caller goes on to reassign — the shorty being
    ;  built for each prototype in turn — would leave every entry
    ;  tracking the last one.
    compute val, value
    match , len
    jyes counted
    compute ln, len
  counted:
    arrange sym, pool#=_n
    compute n, sym
    compute e, 0
    compute found, -1
  scan:
    check e < n
    jno missed
    match , len
    jyes same_length
    arrange sym, pool#=_l_#e
    compute v, sym
    check v = ln
    jno step
  same_length:
    arrange sym, pool#=_v_#e
    compute v, sym
    check v = val
    jno step
    compute found, e
    jump done
  step:
    compute e, e + 1
    jump scan
  missed:
    match =add?, miss
    jno done
    arrange sym, pool#=_v_#n
    publish sym, val
    match , len
    jyes no_length
    arrange sym, pool#=_l_#n
    publish sym, ln
  no_length:
    match , payload
    jyes no_payload
    compute v, payload
    arrange sym, pool#=_i_#n
    publish sym, v
  no_payload:
    compute found, n
    compute n, n + 1
    arrange sym, pool#=_n
    publish sym, n
  done:
    arrange sym, pool#=_found
    publish sym, found
end calminstruction

; ── Ordering ──────────────────────────────────────────────

; One sort for every pool — the dex's, strings among them, and the
; attributes of an AXML element: binary insertion over
; entries 0 to n-1, leaving the entry at position r in ord#r and the
; position of entry e in rank#e.  `less result, keys, x, y` answers
; whether entry x of the pool named by keys goes before entry y;
; it is the only part that knows what an entry is.  Insertion keeps
; the comparisons to a binary search per entry, and pays for it in
; plain integer moves.
;
; fasmg's # joins tokens rather than values, so reaching an entry
; through a computed index means turning that value into a repeat
; counter first, hence the repeat 1s.
;
; Neighbours that are not strictly in order are two equal entries,
; or a slip in the insertion, and either way the error is `what`.
; Two equal keys would take one position between them and leave
; another unfilled, which surfaces as a missing symbol well away
; from the cause, so they are caught here.
macro _order? keys?*, less?*, rank?*, ord?*, n?*, what?*
    __order_m = $00
    repeat n i:$00
        __order_lo = $00
        __order_hi = __order_m
        while __order_lo < __order_hi
            __order_mid = (__order_lo + __order_hi) / $02
            repeat 1 mid:__order_mid
                repeat 1 k:ord#mid
                    less __order_lt, keys, k, i
                end repeat
            end repeat
            if __order_lt
                __order_lo = __order_mid + $01
            else
                __order_hi = __order_mid
            end if
        end while
        __order_j = __order_m
        while __order_j > __order_lo
            repeat 1 a:__order_j, b:__order_j - $01
                ord#a = ord#b
            end repeat
            __order_j = __order_j - $01
        end while
        repeat 1 a:__order_lo
            ord#a = i
        end repeat
        __order_m = __order_m + $01
    end repeat
    if n > $01
        repeat n - $01 r:$00, r1:$01
            repeat 1 x:ord#r, y:ord#r1
                less __order_lt, keys, x, y
                if ~ __order_lt
                    err what
                end if
            end repeat
        end repeat
    end if
    repeat n r:$00
        repeat 1 e:ord#r
            rank#e = r
        end repeat
    end repeat
end macro

; _order's `less` for a pool of numeric keys, keys#0 onwards.
calminstruction _key_less? result*, keys*, x*, y*
    local sym, a, b, r
    arrange sym, keys#x
    compute a, sym
    arrange sym, keys#y
    compute b, sym
    compute r, 0
    check a < b
    jno fin
    compute r, 1
  fin:
    publish result, r
end calminstruction

; ── UTF-16 length ─────────────────────────────────────────
;
;  How many UTF-16 units `len` bytes of UTF-8 in `value` become.
;  A dex string and a resource pool string both lead with this
;  number, and it is not the number of bytes: "Привіт" is six
;  units and twelve bytes.
;
;  Every byte that is not a continuation byte — not 10xxxxxx —
;  starts a character.  A four-byte sequence is a code point above
;  the basic plane, which UTF-16 writes as a surrogate pair, so it
;  counts for two.

calminstruction _utf16_length? result*, value*, len*
    local i, n, b, v, l
    compute v, value
    compute l, len
    compute n, 0
    compute i, 0
  scan:
    check i < l
    jno done
    compute b, (v shr (i * 8)) and $FF
    compute i, i + 1
    check (b and $C0) = $80
    jyes scan
    compute n, n + 1
    check (b and $F8) = $F0
    jno scan
    compute n, n + 1
    jump scan
  done:
    publish result, n
end calminstruction

; ── Alignment ─────────────────────────────────────────────

macro align? target?*, fill?:$00
    _assert_unsigned target
    assert ((bsr target) = (bsf target))
    while ($ mod target)
        db fill
    end while
end macro

end if
