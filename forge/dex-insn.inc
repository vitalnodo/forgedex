; ══════════════════════════════════════════════════════════
;  dex-insn.inc — the instruction set itself
;
;  Two things, and the order matters: first the def_* helpers, each
;  of which knows how to turn a name and an opcode into a macro of
;  the right shape, and then the table of every instruction the DSL
;  speaks, one line apiece.
;
;  The table is the vocabulary.  Reading it top to bottom is how you
;  find out what can be written, and it is meant to be read that
;  way — which is why it is a table and not prose.
;
;  A def_*_ref helper makes an instruction that names a class, a
;  field or a method outright.  The reference feeds the pools and is
;  answered with the rank of what it named, which the pools settle to
;  later in the same pass; dex-ref.inc says how.
; ══════════════════════════════════════════════════════════

;
;  Each format gets a macro that defines instruction macros, so a
;  new opcode is one table line instead of a copied wrapper.

macro def_22c_ref? name?*, op?*
    macro name? a?*, b?*, ref?*&
        _ref_pair ref
        _ref_field __rf_v, __rf_l
        __fmt_22c op, a, b, __ref_field
    end macro
end macro

macro def_21c_ref? name?*, op?*
    macro name? a?*, ref?*&
        _ref_pair ref
        _ref_field __rf_v, __rf_l
        __fmt_21c op, a, __ref_field
    end macro
end macro

; ── The formats that only pass their operands through ─────
;
;  Seventeen of them need nothing but the opcode and the operands
;  handed to the packer, so they are declared here instead of being
;  written out seventeen times.  Reading this table is how you see
;  the shape of the instruction set: what a format is called, and
;  what it takes.
;
;  The parameter list appears twice — once as it is declared, once as
;  it is passed on — because the two are not the same text: the
;  declaration marks each one required, and dropping that would let a
;  missing operand through in silence.
;
;  The helpers below this table are the ones that do more than pass
;  operands on: a wide register that reserves its pair, or a
;  reference that feeds the pools and is answered with a rank.
;  Those are written out in full.

macro _def_plain? helper?*, decl?, pass?, fmt?*
    ;  def_10x takes no operands at all, and `fmt op, <nothing>`
    ;  would leave a dangling comma — so the shape is chosen here,
    ;  once, rather than tested every time an instruction is written.
    match any, pass
        macro helper? name?*, op?*
            macro name? decl
                fmt op, any
            end macro
        end macro
    else
        macro helper? name?*, op?*
            macro name?
                fmt op
            end macro
        end macro
    end match
end macro

_def_plain def_10x, <>                    , <>              , __fmt_10x
_def_plain def_10t, <offset?*>            , <offset>        , __fmt_10t
_def_plain def_20t, <offset?*>            , <offset>        , __fmt_20t
_def_plain def_30t, <offset?*>            , <offset>        , __fmt_30t
_def_plain def_11x, <a?*>                 , <a>             , __fmt_11x
_def_plain def_11n, <a?*, lit?*>          , <a, lit>        , __fmt_11n
_def_plain def_12x, <a?*, b?*>            , <a, b>          , __fmt_12x
_def_plain def_21c, <a?*, idx?*>          , <a, idx>        , __fmt_21c
_def_plain def_21t, <a?*, offset?*>       , <a, offset>     , __fmt_21t
_def_plain def_22c, <a?*, b?*, idx?*>     , <a, b, idx>     , __fmt_22c
_def_plain def_22t, <a?*, b?*, offset?*>  , <a, b, offset>  , __fmt_22t
_def_plain def_23x, <a?*, b?*, c?*>       , <a, b, c>       , __fmt_23x
_def_plain def_22b, <a?*, b?*, lit?*>     , <a, b, lit>     , __fmt_22b
_def_plain def_31i, <a?*, lit?*>          , <a, lit>        , __fmt_31i
_def_plain def_22x, <a?*, b?*>            , <a, b>          , __fmt_22x
_def_plain def_51l, <a?*, lit?*>          , <a, lit>        , __fmt_51l
_def_plain def_35c, <method?*, regs?&>    , <method, regs>  , __fmt_35c
_def_plain def_21s, <a?*, lit?*>          , <a, lit>        , __fmt_21s
_def_plain def_22s, <a?*, b?*, lit?*>     , <a, b, lit>     , __fmt_22s
_def_plain def_32x, <a?*, b?*>            , <a, b>          , __fmt_32x

; ── Wide-operand instructions ─────────────────────────────
;
;  The layout of an instruction says nothing about whether an
;  operand is a pair, so these wrappers say it instead: the suffix
;  reads left to right over the operands, w for a pair and n for a
;  single.  def_12x_wn is a wide destination taking a narrow source,
;  which is what int-to-double is; def_23x_nww is a narrow result
;  from two pairs, which is what comparing two doubles is.
;
;  Only the frame check differs — the bytes emitted are the same,
;  since the second register of a pair is implied by the first.

macro def_11x_w? name?*, op?*
    macro name? a?*
        _use_reg_wide a, $08, 'wide register'
        __fmt_11x op, a
    end macro
end macro

macro def_12x_ww? name?*, op?*
    macro name? a?*, b?*
        _use_reg_wide a, $04, 'wide register'
        _use_reg_wide b, $04, 'wide register'
        __fmt_12x op, a, b
    end macro
end macro

macro def_12x_wn? name?*, op?*
    macro name? a?*, b?*
        _use_reg_wide a, $04, 'wide register'
        __fmt_12x op, a, b
    end macro
end macro

macro def_12x_nw? name?*, op?*
    macro name? a?*, b?*
        _use_reg_wide b, $04, 'wide register'
        __fmt_12x op, a, b
    end macro
end macro

macro def_22x_ww? name?*, op?*
    macro name? a?*, b?*
        _use_reg_wide a, $08, 'wide register'
        _use_reg_wide b, $10, 'wide source register'
        __fmt_22x op, a, b
    end macro
end macro

macro def_23x_www? name?*, op?*
    macro name? a?*, b?*, c?*
        _use_reg_wide a, $08, 'wide register'
        _use_reg_wide b, $08, 'wide register'
        _use_reg_wide c, $08, 'wide register'
        __fmt_23x op, a, b, c
    end macro
end macro

macro def_23x_wnn? name?*, op?*
    macro name? a?*, b?*, c?*
        _use_reg_wide a, $08, 'wide register'
        __fmt_23x op, a, b, c
    end macro
end macro

macro def_23x_wwn? name?*, op?*
    macro name? a?*, b?*, c?*
        _use_reg_wide a, $08, 'wide register'
        _use_reg_wide b, $08, 'wide register'
        __fmt_23x op, a, b, c
    end macro
end macro

macro def_32x_ww? name?*, op?*
    macro name? a?*, b?*
        _use_reg_wide a, $10, 'wide register'
        _use_reg_wide b, $10, 'wide source register'
        __fmt_32x op, a, b
    end macro
end macro

macro def_23x_nww? name?*, op?*
    macro name? a?*, b?*, c?*
        _use_reg_wide b, $08, 'wide register'
        _use_reg_wide c, $08, 'wide register'
        __fmt_23x op, a, b, c
    end macro
end macro

; A wide static field.
macro def_21c_ref_w? name?*, op?*
    macro name? a?*, ref?*&
        _use_reg_wide a, $08, 'wide register'
        _ref_pair ref
        _ref_field __rf_v, __rf_l
        __fmt_21c op, a, __ref_field
    end macro
end macro

; A wide instance field, named at the point of the access like every
; other field reference.
macro def_22c_ref_w? name?*, op?*
    macro name? a?*, b?*, ref?*&
        _use_reg_wide a, $04, 'wide register'
        _ref_pair ref
        _ref_field __rf_v, __rf_l
        __fmt_22c op, a, b, __ref_field
    end macro
end macro

; const takes a full 32 bits, and a float constant is exactly those
; bits.  Handing the literal to dd rather than shifting it apart
; here keeps 24.0 written as 24.0 instead of as $41C00000.
macro const_float? a?*, lit?*
    local ra
    _reg ra, a, $08, 'register'
    dw (ra shl 8) or $14
    dd lit
end macro

; ── Dalvik instructions ───────────────────────────────────

def_10x return_void,        $0E
def_10x          nop,                    $00    ; does nothing; padding, and a branch target that is not one

def_11x move_result,        $0A
def_11x move_result_object, $0C
def_11x return,             $0F
def_11x return_object,      $11
def_11x          throw,                  $27    ; raise the exception in the register
def_11x          monitor_enter,          $1D    ; take the lock on an object
def_11x          monitor_exit,           $1E    ; release it
def_11x          move_exception,         $0D    ; the exception, at the top of a handler

def_11x_w move_result_wide, $0B
def_11x_w        return_wide,            $10    ; return a long or a double

def_11n const_4,            $12

def_12x move,               $01
def_12x move_object,        $07
def_12x array_length,       $21
def_12x add_int_2addr,      $B0
def_12x sub_int_2addr,      $B1
def_12x mul_int_2addr,      $B2
def_12x int_to_float,       $82
def_12x float_to_int,       $87
def_12x          neg_int,                $7B
def_12x          not_int,                $7C
def_12x          neg_float,              $7F
def_12x          int_to_byte,            $8D
def_12x          int_to_char,            $8E
def_12x          int_to_short,           $8F
def_12x          div_int_2addr,          $B3
def_12x          rem_int_2addr,          $B4
def_12x          and_int_2addr,          $B5
def_12x          or_int_2addr,           $B6
def_12x          xor_int_2addr,          $B7
def_12x          shl_int_2addr,          $B8
def_12x          shr_int_2addr,          $B9
def_12x          ushr_int_2addr,         $BA
def_12x          add_float_2addr,        $C6
def_12x          sub_float_2addr,        $C7
def_12x          mul_float_2addr,        $C8
def_12x          div_float_2addr,        $C9
def_12x          rem_float_2addr,        $CA

; A parameter sits high in the frame — above the frame's locals —
; and the nibble formats cannot name it there, so a body that means
; to work with one copies it down first.
def_22x move_from16,        $02
def_22x move_object_from16, $08
def_22x_ww move_wide_from16, $05

def_31i const,              $14
def_51l const_wide,         $18
def_32x          move_16,                $03
def_32x          move_object_16,         $09
def_32x_ww       move_wide_16,           $06
def_21s          const_16,               $13

; The narrower constants, for a value that fits them.  A wide one
; fills the pair, sign-extended from what is written.
macro const_high16? a?*, lit?*
    __fmt_21h $15, a, lit, $10
end macro

macro const_wide_16? a?*, lit?*
    _use_reg_wide a, $08, 'wide register'
    __fmt_21s $16, a, lit
end macro

macro const_wide_32? a?*, lit?*
    _use_reg_wide a, $08, 'wide register'
    _chk_s lit, $20, 'literal'
    __fmt_31i $17, a, lit
end macro

macro const_wide_high16? a?*, lit?*
    _use_reg_wide a, $08, 'wide register'
    __fmt_21h $19, a, lit, $30
end macro

def_10t goto,               $28
def_20t goto_16,            $29
def_30t goto_32,            $2A

; const_string carries its text the way smali writes it.
macro const_string? a?*, text?*&
    _intern __str, add, text, lengthof text
    repeat 1 e:__str_found
        __fmt_21c $1A, a, __srank_#e
    end repeat
end macro

; The same, for a pool past 65536 strings; it costs a unit more.
macro const_string_jumbo? a?*, text?*&
    _intern __str, add, text, lengthof text
    repeat 1 e:__str_found
        __fmt_31c $1B, a, __srank_#e
    end repeat
end macro

macro new_instance? a?*, descriptor?*&
    _ref_type descriptor
    __fmt_21c $22, a, __resolved_type
end macro

; A cast is an instruction rather than a note to the compiler: the
; verifier tracks a register's type, and something arriving as
; Object stays Object until this says otherwise.  It is what every
; getSystemService call needs before the result can be used.
macro check_cast? a?*, descriptor?*&
    _ref_type descriptor
    __fmt_21c $1F, a, __resolved_type
end macro

; const-class names a type the way new-instance does, but leaves the
; Class object itself in the register — which is how an Intent is
; told which activity to start.
macro const_class? a?*, descriptor?*&
    _ref_type descriptor
    __fmt_21c $1C, a, __resolved_type
end macro

; new-array names its element type the way new-instance names a
; class, and takes the length from a register.  The descriptor is
; the array's own, so an int[] is "[I".
macro new_array? a?*, b?*, descriptor?*&
    _ref_type descriptor
    __fmt_22c $23, a, b, __resolved_type
end macro

; instance-of: 1 in a when b holds something of the type, 0 if not.
macro instance_of? a?*, b?*, descriptor?*&
    _ref_type descriptor
    __fmt_22c $20, a, b, __resolved_type
end macro

; filled-new-array: an array of the type, filled from the registers
; given, left for move-result-object.  The type is the array's own,
; "[I"; ART takes only int arrays and arrays of references here.
; The registers are read, not passed to a call, so they take no
; outgoing argument space: outs_size is put back as it was.
macro filled_new_array? descriptor?*, regs?&
    _ref_type descriptor
    __fna_outs = __max_outs
    __fmt_35c $24, __resolved_type, regs
    __max_outs = __fna_outs
end macro

macro filled_new_array_range? descriptor?*, first?*, last?*
    _ref_type descriptor
    __fna_outs = __max_outs
    __fmt_3rc $25, __resolved_type, first, last
    __max_outs = __fna_outs
end macro

def_21c_ref_w    sget_wide,              $61
def_21c_ref_w    sput_wide,              $68
def_21c_ref sget_object,    $62
def_21c_ref      sget,                   $60
def_21c_ref      sget_boolean,           $63
def_21c_ref      sget_byte,              $64
def_21c_ref      sget_char,              $65
def_21c_ref      sget_short,             $66
def_21c_ref      sput,                   $67
def_21c_ref      sput_object,            $69
def_21c_ref      sput_boolean,           $6A
def_21c_ref      sput_byte,              $6B
def_21c_ref      sput_char,              $6C
def_21c_ref      sput_short,             $6D

def_21t if_eqz,             $38
def_21t if_nez,             $39
def_21t if_ltz,             $3A
def_21t if_gez,             $3B
def_21t if_gtz,             $3C
def_21t if_lez,             $3D

def_22c_ref iget,           $52
def_22c_ref iget_object,    $54
def_22c_ref iget_boolean,   $55
def_22c_ref iput,           $59
def_22c_ref iput_object,    $5B
def_22c_ref iput_boolean,   $5C
def_22c_ref      iget_byte,              $56
def_22c_ref      iget_char,              $57
def_22c_ref      iget_short,             $58
def_22c_ref      iput_byte,              $5D
def_22c_ref      iput_char,              $5E
def_22c_ref      iput_short,             $5F
def_22c_ref_w iget_wide,    $53
def_22c_ref_w iput_wide,    $5A

def_22t if_eq,              $32
def_22t if_lt,              $34
def_22t if_ge,              $35
def_22t if_gt,              $36
def_22t if_le,              $37
def_22t          if_ne,                  $33

def_23x aget,               $44
def_23x aget_object,        $46
def_23x aput,               $4B
def_23x aput_object,        $4D
def_23x add_int,            $90
def_23x sub_int,            $91
def_23x mul_int,            $92
def_23x div_int,            $93
def_23x rem_int,            $94
def_23x and_int,            $95
def_23x or_int,             $96
def_23x shl_int,            $98

; Double arithmetic reads exactly like the int kind — three
; registers — and differs only in that each of them is a pair.
def_23x div_float,          $A9
def_23x          xor_int,                $97
def_23x          shr_int,                $99
def_23x          ushr_int,               $9A
def_23x          add_float,              $A6
def_23x          sub_float,              $A7
def_23x          mul_float,              $A8
def_23x          rem_float,              $AA
def_23x          cmpl_float,             $2D
def_23x          cmpg_float,             $2E
def_23x          aget_boolean,           $47
def_23x          aget_byte,              $48
def_23x          aget_char,              $49
def_23x          aget_short,             $4A
def_23x          aput_boolean,           $4E
def_23x          aput_byte,              $4F
def_23x          aput_char,              $50
def_23x          aput_short,             $51

def_23x_wnn      aget_wide,              $45    ; a pair, from an array and an index
def_23x_wnn      aput_wide,              $4C
def_23x_wwn      shl_long,               $A3    ; a long shifted by an int
def_23x_wwn      shr_long,               $A4
def_23x_wwn      ushr_long,              $A5

def_23x_www add_double,     $AB
def_23x_www sub_double,     $AC
def_23x_www mul_double,     $AD
def_23x_www div_double,     $AE
def_23x_www sub_long,       $9C
def_23x_www div_long,       $9E
def_23x_www      add_long,               $9B
def_23x_www      mul_long,               $9D
def_23x_www      rem_long,               $9F
def_23x_www      and_long,               $A0
def_23x_www      or_long,                $A1
def_23x_www      xor_long,               $A2
def_23x_www      rem_double,             $AF

; Doubles have no branch of their own: a comparison leaves -1, 0 or
; 1 in a narrow register and an ordinary if-testz branches on that.
; cmpg answers 1 when either side is NaN, which is what an escape
; test wants — a diverged point should count as escaped.
def_23x_nww cmpg_double,    $30
def_23x_nww cmpl_double,    $2F
def_23x_nww      cmp_long,               $31

def_12x_wn int_to_double,   $83
def_12x_wn float_to_double, $89
def_12x_wn       int_to_long,            $81
def_12x_wn       float_to_long,          $88
def_12x_wn       shl_long_2addr,         $C3    ; a long shifted by an int
def_12x_wn       shr_long_2addr,         $C4
def_12x_wn       ushr_long_2addr,        $C5
def_12x_nw double_to_int,   $8A
def_12x_nw long_to_int,     $84
def_12x_nw       long_to_float,          $85
def_12x_nw       double_to_float,        $8C
def_12x_ww move_wide,       $04
def_12x_ww add_double_2addr, $CB
def_12x_ww mul_double_2addr, $CD
def_12x_ww       neg_long,               $7D
def_12x_ww       not_long,               $7E
def_12x_ww       neg_double,             $80
def_12x_ww       long_to_double,         $86
def_12x_ww       double_to_long,         $8B
def_12x_ww       add_long_2addr,         $BB
def_12x_ww       sub_long_2addr,         $BC
def_12x_ww       mul_long_2addr,         $BD
def_12x_ww       div_long_2addr,         $BE
def_12x_ww       rem_long_2addr,         $BF
def_12x_ww       and_long_2addr,         $C0
def_12x_ww       or_long_2addr,          $C1
def_12x_ww       xor_long_2addr,         $C2
def_12x_ww       sub_double_2addr,       $CC
def_12x_ww       div_double_2addr,       $CE
def_12x_ww       rem_double_2addr,       $CF

def_22s          add_int_lit16,          $D0
def_22s          rsub_int,               $D1    ; the literal minus the register
def_22s          mul_int_lit16,          $D2
def_22s          div_int_lit16,          $D3
def_22s          rem_int_lit16,          $D4
def_22s          and_int_lit16,          $D5
def_22s          or_int_lit16,           $D6
def_22s          xor_int_lit16,          $D7

def_22b add_int_lit8,       $D8
def_22b rsub_int_lit8,      $D9
def_22b mul_int_lit8,       $DA
def_22b div_int_lit8,       $DB
def_22b rem_int_lit8,       $DC
def_22b xor_int_lit8,       $DF
def_22b shl_int_lit8,       $E0
def_22b          and_int_lit8,           $DD
def_22b          or_int_lit8,            $DE
def_22b          shr_int_lit8,           $E1
def_22b          ushr_int_lit8,          $E2

; The invokes name the method outright, as smali writes them.
macro def_35c_ref? name?*, op?*
    macro name? ref?*, regs?&
        _ref_pair ref
        _ref_method __rf_v, __rf_l
        __fmt_35c op, __ref_method, regs
    end macro
end macro

def_35c_ref invoke_virtual,   $6E
def_35c_ref invoke_super,     $6F
def_35c_ref invoke_direct,    $70
def_35c_ref invoke_static,    $71
def_35c_ref invoke_interface, $72

; The range invokes take the arguments as a span of registers,
; written first..last the way smali writes {v3 .. v10}.  Five
; registers is all 35c holds, so anything wider than that — a
; receiver and seven arguments, say — has to come through here,
; which in turn decides where the body keeps those values.
macro def_3rc_ref? name?*, op?*
    macro name? ref?*, first?*, last?*
        _ref_pair ref
        _ref_method __rf_v, __rf_l
        __fmt_3rc op, __ref_method, first, last
    end macro
end macro

def_3rc_ref invoke_virtual_range,   $74
def_3rc_ref invoke_super_range,     $75
def_3rc_ref invoke_direct_range,    $76
def_3rc_ref invoke_static_range,    $77
def_3rc_ref invoke_interface_range, $78

; ── Tables beside the code ────────────────────────────────
;
;  A switch and fill-array-data point at a table, and a try range at
;  its handlers, and all of them are tables kept beside a method's
;  instructions and addressed relative to them.  So there is one way
;  of keeping them: the instruction or directive records numbers, and
;  `end method` and emit_code lay the tables out — the payloads at
;  the end of the body, each four-byte aligned, the try items and
;  handlers after it.  An offset to something not laid out yet is a
;  symbol defined further on, which fasmg takes from the pass before.
;
;      packed_switch   v0, 10, .ten, .eleven, .twelve   ; first key, then a target each
;      sparse_switch   v0, -1, .neg, 100, .hundred      ; key, target, …, in any order
;      fill_array_data v1, 4, 1, 2, 3                   ; element width, then the values
;
;      catch     "Ljava/lang/NumberFormatException;", .try, .try_end, .bad_number
;      catch_all .try, .try_end, .anything
;
;  A target is a label in the body.  The keys of a sparse switch are
;  put in order here, since the format wants them ascending.

__pl_seq = $00

; The offset in code units from the instruction at `at` to a target.
macro _pl_delta? result?*, target?*, at?*
    if ((target) - (at)) and $01
        err 'a target is not on an instruction boundary'
    end if
    result = ((target) - (at)) shr $01
end macro

; 31t: a register and a 32-bit offset to the table.
macro _pl_31t? op?*, a?*, s?*
    local ra
    _reg ra, a, $08, 'register'
    __pl_off = (__payload_#s - __pl_at_#s) shr $01
    dw (ra shl 8) or op, __pl_off and $FFFF, (__pl_off shr $10) and $FFFF
end macro

macro packed_switch? a?*, first?*, targets?*&
    repeat 1 s:__pl_seq
        __pl_kind_#s = $01
        __pl_at_#s = $
        __pl_key_#s = first
        __pl_n_#s = $00
        iterate t?, targets
            repeat 1 j:__pl_n_#s
                _pl_delta __pl_t_#s#_#j, t, __pl_at_#s
            end repeat
            __pl_n_#s = __pl_n_#s + $01
        end iterate
        _pl_31t $2B, a, s
    end repeat
    __pl_seq = __pl_seq + $01
end macro

macro sparse_switch? a?*, cases?*&
    repeat 1 s:__pl_seq
        __pl_kind_#s = $02
        __pl_at_#s = $
        __pl_n_#s = $00
        iterate <key?, t?>, cases
            repeat 1 j:__pl_n_#s
                __pl_k_#s#_#j = key
                _chk_s __pl_k_#s#_#j, $20, 'switch key'
                _pl_delta __pl_t_#s#_#j, t, __pl_at_#s
            end repeat
            __pl_n_#s = __pl_n_#s + $01
        end iterate
        _order __pl_k_#s#_, _key_less, __pl_r_#s#_, __pl_o_#s#_, __pl_n_#s, \
            'two cases of a sparse switch share a key'
        _pl_31t $2C, a, s
    end repeat
    __pl_seq = __pl_seq + $01
end macro

macro fill_array_data? a?*, width?*, values?*&
    if ~ (((width) = $01) | ((width) = $02) | ((width) = $04) | ((width) = $08))
        err 'an element is 1, 2, 4 or 8 bytes wide'
    end if
    repeat 1 s:__pl_seq
        __pl_kind_#s = $03
        __pl_at_#s = $
        __pl_w_#s = width
        __pl_n_#s = $00
        iterate v?, values
            repeat 1 j:__pl_n_#s
                __pl_d_#s#_#j = v
            end repeat
            __pl_n_#s = __pl_n_#s + $01
        end iterate
        _pl_31t $26, a, s
    end repeat
    __pl_seq = __pl_seq + $01
end macro

; A try range and one of its handlers.  Ranges are kept as code
; units; a type of -1 is catch-all.  The order handlers of one range
; are written in is kept, since the first that matches is the one
; taken.
macro _try_add? start?*, finish?*, type?*, handler?*
    if ((start) or (finish) or (handler)) and $01
        err 'a try range or handler is not on an instruction boundary'
    end if
    if (finish) <= (start)
        err 'a try range ends before it starts'
    end if
    repeat 1 k:__m_k
        repeat 1 i:__ctry_n_#k
            __ctry_s_#k#_#i = (start) shr $01
            __ctry_e_#k#_#i = (finish) shr $01
            __ctry_t_#k#_#i = type
            __ctry_h_#k#_#i = (handler) shr $01
        end repeat
        __ctry_n_#k = __ctry_n_#k + $01
    end repeat
end macro

macro catch? descriptor?*, start?*, finish?*, handler?*
    _ref_type descriptor
    _try_add start, finish, __resolved_type, handler
end macro

macro catch_all? start?*, finish?*, handler?*
    _try_add start, finish, -$01, handler
end macro

struc defstring? _value?*, _len?*
    local utf16len?, i?, b?, b1?, b2?, b3?, cp?, hi?, lo?
    _utf16_length utf16len?, _value, _len
    label .
    .utf16_size?: leb128 utf16len?
    ; emit MUTF-8 data: convert 4-byte UTF-8 to surrogate pairs, and
    ; U+0000 to C0 80, the two-byte form, since a 00 byte is where the
    ; reader stops
    i? = 0
    while i < _len
        b? = (_value shr (i * 8)) and $FF
        if b = $00
            db $C0, $80
            i? = i + 1
        else if b < $F0
            db b
            i? = i + 1
        else
            b1? = (_value shr ((i + 1) * 8)) and $FF
            b2? = (_value shr ((i + 2) * 8)) and $FF
            b3? = (_value shr ((i + 3) * 8)) and $FF
            cp? = ((b and $07) shl 18) or ((b1 and $3F) shl 12) or ((b2 and $3F) shl 6) or (b3 and $3F)
            hi? = $D800 or ((cp - $10000) shr 10)
            lo? = $DC00 or ((cp - $10000) and $3FF)
            db $E0 or (hi shr 12)
            db $80 or ((hi shr 6) and $3F)
            db $80 or (hi and $3F)
            db $E0 or (lo shr 12)
            db $80 or ((lo shr 6) and $3F)
            db $80 or (lo and $3F)
            i? = i + 4
        end if
    end while
    db $00
end struc

macro emit_checksums?
    __snapshot_index string_ids_size, __string_id_item
    __snapshot_index type_ids_size,   __type_id_item
    __snapshot_index proto_ids_size,  __proto_id_item
    __snapshot_index field_ids_size,  __field_id_item
    __snapshot_index method_ids_size, __method_id_item
    __snapshot_index class_defs_size, __class_def_item
end macro

; The signature and the checksum are over the finished file, and
; nothing in the file depends on them, so they are worked out once,
; when every pass is done: dex_file hands this to `postpone ?`,
; which fasmg runs after the last pass only.  Done in every pass they
; were most of what was left of a build's time.
macro _dex_checksums?
    load __sha1_payload : (header_end - header_sha1) from __global : header_sha1
    __sha1_result sha1 __sha1_payload
    store __sha1_result : $14 at __global : header_adler32

    load __adler32_payload : (header_end - header_adler32) from __global : header_adler32
    __adler32_result adler32 __adler32_payload
    store __adler32_result : $04 at __global : header_magic_end
end macro
