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
;  v0..v255 covers every format that holds a register in a byte
;  (11x, 21c, 22b).  Formats with a nibble field (11n, 22c, 35c)
;  are limited to v0..v15, which the format macros enforce.
;
;  Defined with := so that assigning to a register name is an
;  error rather than a silent redefinition.

repeat $100 i:$00
    v#i := i
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

macro _use_reg? value?*, bits?*, what?*
    _chk_u value, bits, what
    if (value) > __max_reg
        __max_reg = (value)
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
macro _use_reg_wide? value?*, bits?*, what?*
    _chk_u value, bits, what
    if (value) + $01 > __max_reg
        __max_reg = (value) + $01
    end if
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
    _use_reg a, $08, 'register'
    dw (a shl 8) or op
end macro

macro __fmt_11n? op?*, a?*, lit?*
    _use_reg a, $04, 'register'
    _chk_s lit, $04, 'literal'
    dw ((lit and $0F) shl 12) or (a shl 8) or op
end macro

; 12x: two registers, both in one nibble each.  This is the only
; way to add, subtract or otherwise combine two runtime values —
; every *_lit8 format takes one register and one compile-time
; literal, which cannot express an accumulator like birdY += velY
; where both sides change every frame.
macro __fmt_12x? op?*, a?*, b?*
    _use_reg a, $04, 'register'
    _use_reg b, $04, 'register'
    dw (b shl 12) or (a shl 8) or op
end macro

macro __fmt_21c? op?*, a?*, idx?*
    _use_reg a, $08, 'register'
    _chk_u idx, $10, 'pool index'
    dw (a shl 8) or op, idx
end macro

macro __fmt_21t? op?*, a?*, target?*
    local delta
    _use_reg a, $08, 'register'
    _branch_delta delta, target
    _chk_s delta, $10, 'branch offset'
    dw (a shl 8) or op, delta
end macro

macro __fmt_22c? op?*, a?*, b?*, idx?*
    _use_reg a, $04, 'register'
    _use_reg b, $04, 'register'
    _chk_u idx, $10, 'pool index'
    dw (b shl 12) or (a shl 8) or op, idx
end macro

macro __fmt_22t? op?*, a?*, b?*, target?*
    local delta
    _use_reg a, $04, 'register'
    _use_reg b, $04, 'register'
    _branch_delta delta, target
    _chk_s delta, $10, 'branch offset'
    dw (b shl 12) or (a shl 8) or op, delta
end macro

macro __fmt_22b? op?*, a?*, b?*, lit?*
    _use_reg a, $08, 'register'
    _use_reg b, $08, 'register'
    _chk_s lit, $08, 'literal'
    dw (a shl 8) or op, ((lit and $FF) shl 8) or b
end macro

; 22x: a destination in a byte and a source in a full word, which
; is how a value is fetched down from a register the nibble formats
; cannot name.  A method with many parameters puts them high in the
; frame, out of reach of the 12x and 22c formats, so the body copies
; them down before it starts.
macro __fmt_22x? op?*, a?*, b?*
    _use_reg a, $08, 'register'
    _use_reg b, $10, 'source register'
    dw (a shl 8) or op, b
end macro

; 23x: three registers, a byte each.  This is what a binary
; operation looks like when neither operand is known at assembly
; time and the result goes somewhere else — shifting by a value
; that is itself computed, for instance.
macro __fmt_23x? op?*, a?*, b?*, c?*
    _use_reg a, $08, 'register'
    _use_reg b, $08, 'register'
    _use_reg c, $08, 'register'
    dw (a shl 8) or op, (c shl 8) or b
end macro

; 31i: a register and a full 32-bit literal, which is how a colour
; or a float constant is written.
macro __fmt_31i? op?*, a?*, lit?*
    _use_reg a, $08, 'register'
    _chk_u (lit) and $FFFFFFFF, $20, 'literal'
    dw (a shl 8) or op, (lit) and $FFFF, ((lit) shr $10) and $FFFF
end macro

; 51l: a register pair and a full 64-bit literal — the only way to
; get a double into the frame.  The literal is handed to dq rather
; than taken apart here, so an integer arrives as an integer and
; 4.0 arrives as the IEEE-754 double the arithmetic expects.
macro __fmt_51l? op?*, a?*, lit?*
    _use_reg_wide a, $08, 'wide register'
    dw (a shl 8) or op
    dq lit
end macro

macro __fmt_35c? op?*, idx?*, regs?&
    local count?, c?, d?, e?, f?, g?
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
            _use_reg reg, $04, 'argument register'
            match =1, %
                c? = reg
            end match
            match =2, %
                d? = reg
            end match
            match =3, %
                e? = reg
            end match
            match =4, %
                f? = reg
            end match
            match =5, %
                g? = reg
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
    local count?
    _chk_u idx, $10, 'pool index'
    _use_reg first, $10, 'first argument register'
    _use_reg last, $10, 'last argument register'
    if (last) < (first)
        err 'the argument range ends before it starts'
    end if
    count? = (last) - (first) + $01
    _chk_u count?, $08, 'argument count'
    if count? > __max_outs
        __max_outs = count?
    end if
    dw (count? shl 8) or op, idx, first
end macro
 
