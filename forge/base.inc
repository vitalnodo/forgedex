; ══════════════════════════════════════════════════════════
;  base.inc — what every forged format needs underneath
;
;  A dex, a resources.arsc and a binary AXML have almost nothing
;  in common, but all three are laid out by writing fields of a
;  fixed width and aligning to a boundary.  That is all this is.
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

; ── Alignment ─────────────────────────────────────────────

macro align? target?*, fill?:$00
    _assert_unsigned target
    assert ((bsr target) = (bsf target))
    while ($ mod target)
        db fill
    end while
end macro

end if
