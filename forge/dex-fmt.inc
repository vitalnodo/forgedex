; ══════════════════════════════════════════════════════════
;  dex-fmt.inc — how an instruction is packed into bytes
;
;  Dalvik packs operands into fixed-width nibbles and bytes, and the
;  format names say which: 11n is one register and a four-bit
;  literal, 22b is two registers and a byte, 35c is up to five
;  registers and a pool index.  One macro per format, and every
;  operand goes through a range check first — without them a
;  register that does not fit its nibble is written truncated and the
;  instruction quietly means something else.
;
;  Branches take a label, not a counted offset: the body is assembled
;  in a virtual block, so `$` is the address of the instruction and
;  _branch_delta works the distance out itself.
;
;  Reading an instruction's operands also decides two header fields —
;  outs_size comes from the widest invoke, and a register outside the
;  declared frame is an error rather than a corrupt dex.
; ══════════════════════════════════════════════════════════

;
;  A register is not a number.  v0 … v255 and p0 … are values of
;  their own kind, __vreg plus the register's number, so an operand
;  says what it is: `move v1, 3` is an error rather than a move from
;  v3, and `const_4 v0, v1` is an error rather than a load of 1.
;  The nibble formats (11n, 22c, 35c) reach v0 … v15 and the byte
;  ones v0 … v255; the few that reach further — 22x and 32x, and the
;  ranges of 3rc — take v0 + n for a register past the names.
;
;  Defined with := so that assigning to a register name is an
;  error rather than a silent redefinition.

element __vreg?

repeat $100 i:$00
    v#i := __vreg + i
end repeat


; ── Dalvik instruction formats ────────────────────────────
;
;  Dalvik packs operands into fixed-width nibbles and bytes.
;  Without the _chk_u / _chk_s checks an out-of-range register
;  or literal is silently truncated, producing well-formed but
;  wrong bytecode that only misbehaves at run time on the device.

; Branch instructions take a label, not a hand-counted offset.
; Dalvik measures a branch in 16-bit code units from the address
; of the branch instruction itself, and method assembles a
; body into `virtual at $00`, so $ here is that address.
;
; Every branch format is fixed-width, so the body size does not
; depend on any offset and no branch relaxation is needed.

; Registers touched by a method decide two header fields, so the
; format macros record what they see: __max_reg validates the
; declared registers_size, __max_outs supplies outs_size.  Both are
; reset by method and read by end method.

__max_reg = -$01
__max_outs = $00

; The register an operand names, as the number an instruction
; field holds.  Anything that is not a register stops here, whatever
; it would have fitted in.
macro _reg? result?*, value?*, bits?*, what?*
    if ~ (value) relativeto __vreg
        err what, ' expected: v0 … or p0 …, not a number'
    end if
    result = (value) - __vreg
    _chk_u result, bits, what
    if result > __max_reg
        __max_reg = result
    end if
end macro

; A long or a double lives in a pair, vN and vN+1, and only the
; first of the two is ever written into an instruction.  The second
; is occupied all the same, so the frame has to be measured against
; it: a double placed in the last register of the frame runs off the
; end, which the verifier rejects on the device rather than here.
;
; Which operands are a pair is a property of the opcode, not of the
; layout, so the format macros stay unaware and each opcode's own
; wrapper widens the ones it knows about.  Raising __max_reg twice
; for the same register is harmless — it only ever moves up.
macro _reg_wide? result?*, value?*, bits?*, what?*
    _reg result, value, bits, what
    if result + $01 > __max_reg
        __max_reg = result + $01
    end if
end macro

; The same check, for a wrapper that only has to say which of its
; operands are pairs and leaves the encoding to the format.
macro _use_reg_wide? value?*, bits?*, what?*
    local r
    _reg_wide r, value, bits, what
end macro

macro _branch_delta? result?*, target?*
    if (target - $) and $01
        err 'branch target is not on an instruction boundary'
    end if
    result = (target - $) / $02
    if ~ result
        err 'branch to the branch instruction itself'
    end if
end macro

macro __fmt_10x? op?*
    dw op
end macro

macro __fmt_10t? op?*, target?*
    local delta
    _branch_delta delta, target
    _chk_s delta, $08, 'branch offset'
    dw ((delta and $FF) shl 8) or op
end macro

; 20t and 30t: a jump whose offset does not fit the byte that 10t
; carries.  Every branch format here is fixed-width and none of them
; is chosen automatically, so a loop that outgrows its goto says so
; at assembly time rather than silently wrapping — which is what the
; range check on 10t is there to do.
macro __fmt_20t? op?*, target?*
    local delta
    _branch_delta delta, target
    _chk_s delta, $10, 'branch offset'
    dw op, delta
end macro

macro __fmt_30t? op?*, target?*
    local delta
    _branch_delta delta, target
    _chk_s delta, $20, 'branch offset'
    dw op, (delta) and $FFFF, ((delta) shr $10) and $FFFF
end macro

macro __fmt_11x? op?*, a?*
    local ra
    _reg ra, a, $08, 'register'
    dw (ra shl 8) or op
end macro

macro __fmt_11n? op?*, a?*, lit?*
    local ra
    _reg ra, a, $04, 'register'
    _chk_s lit, $04, 'literal'
    dw ((lit and $0F) shl 12) or (ra shl 8) or op
end macro

; 12x: two registers, both in one nibble each.  This is the only
; way to add, subtract or otherwise combine two runtime values —
; every *_lit8 format takes one register and one compile-time
; literal, which cannot express an accumulator like birdY += velY
; where both sides change every frame.
macro __fmt_12x? op?*, a?*, b?*
    local ra, rb
    _reg ra, a, $04, 'register'
    _reg rb, b, $04, 'register'
    dw (rb shl 12) or (ra shl 8) or op
end macro

macro __fmt_21c? op?*, a?*, idx?*
    local ra
    _reg ra, a, $08, 'register'
    _chk_u idx, $10, 'pool index'
    dw (ra shl 8) or op, idx
end macro

; 21s: a register and a signed 16-bit literal, sign-extended to 32
; or 64 bits by the opcode.
macro __fmt_21s? op?*, a?*, lit?*
    local ra
    _reg ra, a, $08, 'register'
    _chk_s lit, $10, 'literal'
    dw (ra shl 8) or op, (lit) and $FFFF
end macro

; 21h: a register and the top 16 bits of a literal whose other bits
; are all zero — shift is 16 for a 32-bit value and 48 for a 64-bit
; one.  It is how 1.0f, $3F800000, fits in two units instead of
; three.  The literal may be written as the bits themselves or as a
; negative number, so both $BF800000 and -$40800000 are -1.0f.
macro __fmt_21h? op?*, a?*, lit?*, shift?*
    local ra
    _reg ra, a, $08, 'register'
    if (lit) and (($01 shl (shift)) - $01)
        err 'only the top 16 bits of this literal may be set'
    end if
    if ~ ((((lit) shr (shift)) >= -$8000) & (((lit) shr (shift)) <= $FFFF))
        err 'literal out of range'
    end if
    dw (ra shl 8) or op, ((lit) shr (shift)) and $FFFF
end macro

macro __fmt_21t? op?*, a?*, target?*
    local ra
    local delta
    _reg ra, a, $08, 'register'
    _branch_delta delta, target
    _chk_s delta, $10, 'branch offset'
    dw (ra shl 8) or op, delta
end macro

macro __fmt_22c? op?*, a?*, b?*, idx?*
    local ra, rb
    _reg ra, a, $04, 'register'
    _reg rb, b, $04, 'register'
    _chk_u idx, $10, 'pool index'
    dw (rb shl 12) or (ra shl 8) or op, idx
end macro

macro __fmt_22t? op?*, a?*, b?*, target?*
    local ra, rb
    local delta
    _reg ra, a, $04, 'register'
    _reg rb, b, $04, 'register'
    _branch_delta delta, target
    _chk_s delta, $10, 'branch offset'
    dw (rb shl 12) or (ra shl 8) or op, delta
end macro

macro __fmt_22b? op?*, a?*, b?*, lit?*
    local ra, rb
    _reg ra, a, $08, 'register'
    _reg rb, b, $08, 'register'
    _chk_s lit, $08, 'literal'
    dw (ra shl 8) or op, ((lit and $FF) shl 8) or rb
end macro

; 22s: two registers in a nibble each and a signed 16-bit literal,
; the lit16 arithmetic.
macro __fmt_22s? op?*, a?*, b?*, lit?*
    local ra, rb
    _reg ra, a, $04, 'register'
    _reg rb, b, $04, 'register'
    _chk_s lit, $10, 'literal'
    dw (rb shl 12) or (ra shl 8) or op, (lit) and $FFFF
end macro

; 32x: two registers of a full word each — a move between any two
; registers of the frame, however high.
macro __fmt_32x? op?*, a?*, b?*
    local ra, rb
    _reg ra, a, $10, 'register'
    _reg rb, b, $10, 'source register'
    dw op, ra, rb
end macro

; 31c: a register and a pool index of 32 bits, for a string past the
; first 65536.
macro __fmt_31c? op?*, a?*, idx?*
    local ra
    _reg ra, a, $08, 'register'
    _chk_u idx, $20, 'pool index'
    dw (ra shl 8) or op, (idx) and $FFFF, ((idx) shr $10) and $FFFF
end macro

; 22x: a destination in a byte and a source in a full word, which
; is how a value is fetched down from a register the nibble formats
; cannot name.  A method with many parameters puts them high in the
; frame, out of reach of the 12x and 22c formats, so the body copies
; them down before it starts.
macro __fmt_22x? op?*, a?*, b?*
    local ra, rb
    _reg ra, a, $08, 'register'
    _reg rb, b, $10, 'source register'
    dw (ra shl 8) or op, rb
end macro

; 23x: three registers, a byte each.  This is what a binary
; operation looks like when neither operand is known at assembly
; time and the result goes somewhere else — shifting by a value
; that is itself computed, for instance.
macro __fmt_23x? op?*, a?*, b?*, c?*
    local ra, rb, rc
    _reg ra, a, $08, 'register'
    _reg rb, b, $08, 'register'
    _reg rc, c, $08, 'register'
    dw (ra shl 8) or op, (rc shl 8) or rb
end macro

; 31i: a register and a full 32-bit literal, which is how a colour
; or a float constant is written.
macro __fmt_31i? op?*, a?*, lit?*
    local ra
    _reg ra, a, $08, 'register'
    _chk_u (lit) and $FFFFFFFF, $20, 'literal'
    dw (ra shl 8) or op, (lit) and $FFFF, ((lit) shr $10) and $FFFF
end macro

; 51l: a register pair and a full 64-bit literal — the only way to
; get a double into the frame.  The literal is handed to dq rather
; than taken apart here, so an integer arrives as an integer and
; 4.0 arrives as the IEEE-754 double the arithmetic expects.
macro __fmt_51l? op?*, a?*, lit?*
    local ra
    _reg_wide ra, a, $08, 'wide register'
    dw (ra shl 8) or op
    dq lit
end macro

macro __fmt_35c? op?*, idx?*, regs?&
    local count?, r?, c?, d?, e?, f?, g?
    c? = 0
    d? = 0
    e? = 0
    f? = 0
    g? = 0
    count? = 0
    _chk_u idx, $10, 'pool index'
    ; A static method may take nothing at all, and an empty list is
    ; not something iterate can be handed directly.
    match any?, regs
        iterate reg?, regs
            count? = count? + 1
            _reg r?, reg, $04, 'argument register'
            match =1, %
                c? = r?
            end match
            match =2, %
                d? = r?
            end match
            match =3, %
                e? = r?
            end match
            match =4, %
                f? = r?
            end match
            match =5, %
                g? = r?
            end match
        end iterate
    end match
    if count > $05
        err 'format 35c takes at most 5 argument registers'
    end if
    if count > __max_outs
        __max_outs = count
    end if
    dw (count? shl 12) or (g? shl 8) or op, idx, (f? shl 12) or (e? shl 8) or (d? shl 4) or c?
end macro

; 3rc: the arguments are a run of consecutive registers rather than
; a handful named one by one, which is the only way to call anything
; taking more than the five that fit in 35c — Bitmap.setPixels takes
; seven and a receiver.  Written first..last, the way smali writes
; {v3 .. v10}, so the count is derived and cannot disagree with the
; registers actually meant.
;
; A wide argument still occupies two of the run; naming the last
; register covers it without the caller saying so.
macro __fmt_3rc? op?*, idx?*, first?*, last?*
    local count?, rf?, rl?
    _chk_u idx, $10, 'pool index'
    _reg rf?, first, $10, 'first argument register'
    _reg rl?, last, $10, 'last argument register'
    if rl? < rf?
        err 'the argument range ends before it starts'
    end if
    count? = rl? - rf? + $01
    _chk_u count?, $08, 'argument count'
    if count? > __max_outs
        __max_outs = count?
    end if
    dw (count? shl 8) or op, idx, rf?
end macro
 
