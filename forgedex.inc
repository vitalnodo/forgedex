; ══════════════════════════════════════════════════════════
;  forgedex.inc — hand-forging a DEX file with a macro assembler
;
;  Include this file at the top of an example:
;
;      format binary as "dex"
;      org $00
;      include '../../forgedex.inc'
;
;  This file is the map, not the machinery.  Every part lives in one
;  of its own, and they are included below in the order you would
;  meet them reading the format:
;
;    forge/base.inc         width-checked emitters and align, and the
;                           pools: _intern, _order, the UTF-16 count
;                           and the string order ART checks.  Shared
;                           with forgeres.inc and forgeaxml.inc.
;
;    forge/dex-hash.inc     the Adler-32 and SHA-1 the header carries,
;                           computed over the file being written.
;
;    forge/dex-const.inc    what the format fixes — magic, access
;                           flags, map section types — with the index
;                           bookkeeping and the sortedness checks.
;
;    forge/dex-struct.inc   one struc per item: the five id pools,
;                           class_def, the lists, code_item.
;
;    forge/dex-fmt.inc      v0..v255, and how each instruction format
;                           packs its operands into nibbles.
;
;    forge/dex-ref.inc      reading "Lclass;->name(args)ret" back into
;                           what it names, and answering an
;                           instruction with the rank of each part.
;
;    forge/dex-insn.inc     the def_* helpers, then the table of every
;                           instruction the DSL speaks.  Read that
;                           table to find out what can be written.
;
;    forge/dex-file.inc     the header, the order of the sections, and
;                           the map saying where each of them went.
;
;    forge/dex-strings.inc  the string pool's declarations, and the
;                           lists the one walk reads.
;
;    forge/dex-class.inc    defclass, method … end method and the
;                           class data — what an example writes.
;
;    forge/dex-pools.inc    deftype, deffield, defmethod, defproto,
;                           and the one walk that collects every pool
;                           before they are put in order bottom up.
;
;  To write an example you need dex-insn.inc for the vocabulary, and
;  dex-class.inc and dex-pools.inc for how to declare things.  The
;  rest is how the format is satisfied.
; ══════════════════════════════════════════════════════════

if ~ definite __forgedex_included
__forgedex_included := $01

include 'forge/base.inc'

;  LEB128 is dex's own — neither resource format uses it, so it stays
;  here rather than in base.
macro leb128? list?*&
    local value?, least?, more?
    iterate item?*, list
        value? = item
        more? = $01
        while (more)
            least? = value and $7F
            value? = value shr $07
            more? = $00
            if ~ ((value = $00) | ((value = -$01) & (least = $7F)))
                least? = least or $80
                more? = $01
            end if
            db least
        end while
    end iterate
end macro

;  The signed form, which stops where the sign bit of the last group
;  agrees with what is left: 64 needs two bytes here and one above.
macro sleb128? list?*&
    local value?, least?, more?
    iterate item?*, list
        value? = item
        more? = $01
        while (more)
            least? = value and $7F
            value? = value shr $07
            more? = $00
            if ~ (((value = $00) & ((least and $40) = $00)) | ((value = -$01) & ((least and $40) <> $00)))
                least? = least or $80
                more? = $01
            end if
            db least
        end while
    end iterate
end macro

include 'forge/dex-hash.inc'
include 'forge/dex-const.inc'
include 'forge/dex-struct.inc'
include 'forge/dex-fmt.inc'
include 'forge/dex-ref.inc'
include 'forge/dex-insn.inc'
include 'forge/dex-file.inc'
include 'forge/dex-strings.inc'
include 'forge/dex-class.inc'
include 'forge/dex-pools.inc'

end if
