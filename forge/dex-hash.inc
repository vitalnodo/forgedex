; ══════════════════════════════════════════════════════════
;  dex-hash.inc — the two checksums a dex carries, at assembly time
;
;  A dex header holds an Adler-32 over everything after it and a
;  SHA-1 over everything after that.  Both are computed here, by the
;  assembler, over the file it is in the middle of writing — which is
;  why they are structures that take a payload rather than functions
;  that take a buffer.
;
;  Both run in every pass the assembler makes, over the whole file,
;  so they are what a build mostly waits on.  SHA-1's rounds are a
;  CALM instruction for that reason: as macros they were half the
;  time of the largest examples.
; ══════════════════════════════════════════════════════════

; ── SHA-1 (assembly-time) ─────────────────────────────────

; One 64-byte block into the state __sha_h0 … __sha_h4, each named
; through arrange: a bare global reads in this instruction's own
; namespace.  The block is
; one number, its first byte lowest, as load gives it; each word is
; big-endian, so it is turned round.  The schedule lives in __sha_w_0
; onwards, since a CALM instruction has no arrays of its own.
calminstruction _sha1_block? block*
    local blk, i, w, x, sym, a, b, c, d, e, f, k, t, m
    compute m, $FFFFFFFF
    compute blk, block
    compute i, 0
  load_words:
    check i < 16
    jno extend
    compute w, ((blk shr (32 * i)) and m) bswap 4
    arrange sym, =__sha_w_#i
    publish sym, w
    compute i, i + 1
    jump load_words
  extend:
    check i < 80
    jno rounds
    compute x, 0
    compute t, i - 3
    arrange sym, =__sha_w_#t
    compute x, x xor sym
    compute t, i - 8
    arrange sym, =__sha_w_#t
    compute x, x xor sym
    compute t, i - 14
    arrange sym, =__sha_w_#t
    compute x, x xor sym
    compute t, i - 16
    arrange sym, =__sha_w_#t
    compute x, x xor sym
    compute w, ((x shl 1) or (x shr 31)) and m
    arrange sym, =__sha_w_#i
    publish sym, w
    compute i, i + 1
    jump extend
  rounds:
    arrange sym, =__sha_h0
    compute a, sym
    arrange sym, =__sha_h1
    compute b, sym
    arrange sym, =__sha_h2
    compute c, sym
    arrange sym, =__sha_h3
    compute d, sym
    arrange sym, =__sha_h4
    compute e, sym
    compute i, 0
  round:
    check i < 80
    jno fold
    check i < 20
    jno r2
    compute f, (b and c) or ((b xor m) and d)
    compute k, $5A827999
    jump mix
  r2:
    check i < 40
    jno r3
    compute f, b xor c xor d
    compute k, $6ED9EBA1
    jump mix
  r3:
    check i < 60
    jno r4
    compute f, (b and c) or (b and d) or (c and d)
    compute k, $8F1BBCDC
    jump mix
  r4:
    compute f, b xor c xor d
    compute k, $CA62C1D6
  mix:
    arrange sym, =__sha_w_#i
    compute t, ((((a shl 5) or (a shr 27)) and m) + f + e + k + sym) and m
    compute e, d
    compute d, c
    compute c, ((b shl 30) or (b shr 2)) and m
    compute b, a
    compute a, t
    compute i, i + 1
    jump round
  fold:
    arrange sym, =__sha_h0
    compute a, (sym + a) and m
    publish sym, a
    arrange sym, =__sha_h1
    compute b, (sym + b) and m
    publish sym, b
    arrange sym, =__sha_h2
    compute c, (sym + c) and m
    publish sym, c
    arrange sym, =__sha_h3
    compute d, (sym + d) and m
    publish sym, d
    arrange sym, =__sha_h4
    compute e, (sym + e) and m
    publish sym, e
end calminstruction

struc sha1? payload?*
    local container?, size?, chunk?, block?
    __sha_h0 = $67452301
    __sha_h1 = $EFCDAB89
    __sha_h2 = $98BADCFE
    __sha_h3 = $10325476
    __sha_h4 = $C3D2E1F0
    size? = lengthof payload
    chunk? = $00
    virtual at $00
    container?::
        db payload, $80
        ;  Zeros up to eight short of a block.  None at all is a
        ;  length of its own — 55 more than a multiple of 64 — and
        ;  emit refuses a size of nothing, so it is asked only for
        ;  some.
        if (($ + $08) mod $40)
            emit ($40 - (($ + $08) mod $40)) : $00
        end if
        dq (size * $08) bswap $08
        size? = $
    end virtual
    while (chunk < size)
        load block? : $40 from container:chunk
        _sha1_block block
        chunk? = chunk + $40
    end while
    ;  The digest is h0 first, each word big-endian, and a value is
    ;  stored low byte first, so the number is turned round to come
    ;  out as the digest reads.  Without it the signature was
    ;  written backwards.
    . = ((__sha_h4) or (__sha_h3 shl $20) or (__sha_h2 shl $40) or (__sha_h1 shl $60) or (__sha_h0 shl $80)) bswap $14
end struc

; ── Adler-32 (assembly-time) ──────────────────────────────

; Up to 64 bytes, as one number with its first byte lowest, into
; the running sums __adler_a and __adler_b.
calminstruction _adler32_block? block*, count*
    local blk, n, i, a, b, sym
    compute blk, block
    compute n, count
    arrange sym, =__adler_a
    compute a, sym
    arrange sym, =__adler_b
    compute b, sym
    compute i, 0
  byte:
    check i < n
    jno fold
    compute a, (a + ((blk shr (i * 8)) and $FF)) mod $FFF1
    compute b, (b + a) mod $FFF1
    compute i, i + 1
    jump byte
  fold:
    arrange sym, =__adler_a
    publish sym, a
    arrange sym, =__adler_b
    publish sym, b
end calminstruction

; Read a block at a time rather than taking the payload apart byte
; by byte: a byte out of a number the size of the file costs a shift
; of the whole file, which made the checksum quadratic in its size.
struc adler32? payload?*
    local container?, size?, pos?, part?, block?
    __adler_a = $01
    __adler_b = $00
    virtual at $00
    container?::
        db payload
        size? = $
    end virtual
    pos? = $00
    while pos < size
        part? = size - pos
        if part > $40
            part? = $40
        end if
        load block? : part from container:pos
        _adler32_block block, part
        pos? = pos + part
    end while
    . = (__adler_b shl $10) or __adler_a
end struc
