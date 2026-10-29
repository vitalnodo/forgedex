; ══════════════════════════════════════════════════════════
;  dex-ref.inc — turning a written reference into pool indices
;
;  An instruction names what it means outright, the way smali writes
;  it:
;
;      invoke_virtual "Landroid/widget/TextView;->setText(...)V"
;
;  and everything needed to emit that — the class, the name, the
;  prototype, the shorty, every parameter type — has to be read back
;  out of those characters.  This file is that reading.
;
;  It comes in three layers, which the old section headings did not
;  make clear:
;
;    • finding an entry in a pool that is already built — one scan
;      for strings, one for anything ranked by a key;
;    • cutting a reference apart — where the arrow is, where the
;      parentheses are, where one descriptor ends and the next
;      begins;
;    • what each kind of reference contributes: it feeds the pools,
;      and is answered with the rank what it named will have.
;
;  Most of the scanning is CALM rather than macros.  That is not
;  style: the string comparison alone was measured at fourteen times
;  faster, and it runs once per comparison of a binary insertion sort
;  over every string in the file.
; ══════════════════════════════════════════════════════════

;
;  An instruction can name what it means outright:
;
;      new_instance v0, "Landroid/widget/Button;"
;
;  rather than pointing at a type declared elsewhere.  The
;  descriptor is read, looked up in the string pool, and turned
;  into the type index.
;
;  Looking up, not registering: whatever the code names was put in
;  its pool while the lists were collected, the method bodies with
;  them, and the pools are settled before the code is assembled for
;  real.  So by then a lookup finds, and a miss is a mistake.

; The pools are settled by the time anything looks a string up, so a
; miss is a mistake rather than a new entry.
macro _lookup_string? result?*, value?*, len?*
    _find_ranked result, __str, __srank_, value, len
    if result < $00
        err 'no string in the pool matches this text'
    end if
end macro

; ── Finding an entry in a ranked pool ─────────────────────
;
;  Every sorted pool — strings, types, fields, methods — keeps a
;  table of ranks beside its entries, and finding an entry is the
;  same in all of them: _intern finds where it was added, and the
;  rank at that index is where the sort put it.  A miss answers -1.
;  len is for the string pool, the one whose entries keep a length.
;
;  A key is one integer even when it names three things: the callers
;  below pack class, name and prototype into one, major part first,
;  so a single comparison covers the whole reference.
macro _find_ranked? result?*, pool?*, rank?*, key?*, len?
    _intern pool, find, key, len
    if pool#_found < $00
        result = -1
    else
        repeat 1, e:pool#_found
            result = rank#e
        end repeat
    end if
end macro

macro _lookup_type? result?*, sidx?*
    _find_ranked result, __type, __trank_, sidx
    if result < $00
        err 'no type is declared with this descriptor'
    end if
end macro

; ── Answering with a rank ─────────────────────────────────
;
;  An instruction interns what it names, the way a declaration does,
;  and is answered with the rank of what it interned.  The rank is
;  only set once everything is in, by _dex_settle, later in the same
;  pass — and that is fine: fasmg takes a symbol defined further on
;  from the pass before, and runs another when it has to.  So the
;  code is assembled once, while the lists are collected, and with
;  its final indices.

; A type, answered in __resolved_type.
macro _ref_type? descriptor?*&
    _use_type descriptor, lengthof descriptor
    repeat 1 t:__type_found
        __resolved_type = __trank_#t
    end repeat
end macro

; A method, answered in __ref_method.
macro _ref_method? ref?*, n?*
    _use_method_ref ref, n
    repeat 1 k:__method_found
        __ref_method = __mrank_#k
    end repeat
end macro

; A field, answered in __ref_field.  A field is only declared, never
; added by being named, and it may be declared after the code that
; names it — so a reference is kept by its parts, and _dex_settle
; matches it to a declared field once all are in.  -1 is no match,
; and the error is here, where the reference is.
macro _ref_field? ref?*, n?*
    _use_field_ref_types ref, n
    _intern __fref, add, (__uf_cls shl $40) or (__uf_nm shl $20) or __uf_ty
    repeat 1 r:__fref_found
        if __frefrank_#r < $00
            err 'no field is declared with this reference'
        end if
        __ref_field = __frefrank_#r
    end repeat
end macro

; ── Resolving a method reference ──────────────────────────
;
;      invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v1, v0
;
;  The reference is split on -> ( and ), each descriptor is turned
;  into a type index, and the prototype and the method are found by
;  those.  As elsewhere this finds rather than adds.

calminstruction _find_byte? result*, v*, l*, ch*, start*
    local i, b, sym, c
    compute c, ch
    compute i, start
  next:
    check i < l
    jno miss
    compute b, (v shr (i * 8)) and $FF
    check b = c
    jyes hit
    compute i, i + 1
    jump next
  miss:
    compute i, -1
  hit:
    arrange sym, result
    publish sym, i
end calminstruction

calminstruction _slice? outv*, outl*, sv*, sl*, from*, to*
    local v, n, sym, f
    compute f, from
    compute n, to - f
    compute v, (sv shr (f * 8)) and ((1 shl (n * 8)) - 1)
    arrange sym, outv
    publish sym, v
    arrange sym, outl
    publish sym, n
end calminstruction

; One past the descriptor that starts at at.
calminstruction _desc_end? result*, v*, l*, at*
    local i, b, sym
    compute i, at
  arr:
    compute b, (v shr (i * 8)) and $FF
    check b = $5B
    jno head
    compute i, i + 1
    jump arr
  head:
    check b = $4C
    jyes isref
    compute i, i + 1
    jump fin
  isref:
    compute i, i + 1
  scan:
    check i < l
    jno fin
    compute b, (v shr (i * 8)) and $FF
    compute i, i + 1
    check b = $3B
    jno scan
  fin:
    arrange sym, result
    publish sym, i
end calminstruction

; Split a reference into __ref_arrow, __ref_open and __ref_close, the
; way _split_field_ref does for a field.
macro _split_method_ref? ref?*, n?*
    _find_byte __ref_arrow, ref, n, $2D, $00
    _find_byte __ref_open,  ref, n, $28, $00
    _find_byte __ref_close, ref, n, $29, $00
    if (__ref_arrow < $00) | (__ref_open < $00) | (__ref_close < $00)
        err 'a method reference reads Lclass;->name(parameters)return'
    end if
end macro

; Walk the descriptors packed between from and to, handing each to a
; macro of the caller's own as a value and a length.  Where one
; descriptor ends and the next begins is not obvious — an array is a
; run of [ and a reference runs to its ; — so _desc_end answers it,
; and answering it in one place is the point of this.
macro _walk_descs? ref?*, n?*, from?*, to?*, handler?*
    __ref_at = from
    while __ref_at < to
        _desc_end __ref_to, ref, n, __ref_at
        _slice __ref_v, __ref_l, ref, n, __ref_at, __ref_to
        handler __ref_v, __ref_l
        __ref_at = __ref_to
    end while
end macro

; Split a reference into the descriptors of its signature.
;
; Two counts come out of this, and they are not the same number.
; __ref_n is how many parameters there are, which is what the
; prototype pool matches on.  __ref_nreg is how many registers they
; occupy, which is what the frame is measured in: a long or a double
; arrives as a pair.  Only a bare J or D is wide — `[D` is a
; reference to an array and fits in one register like any other.
; One parameter of a signature being measured: kept for the shorty,
; and counted as one register or as two.
macro _sig_param? v?*, l?*
    repeat 1 j:__ref_n
        __ref_pv_#j = v
        __ref_pl_#j = l
    end repeat
    if (l = $01) & ((v = $4A) | (v = $44))
        __ref_nreg = __ref_nreg + $02
    else
        __ref_nreg = __ref_nreg + $01
    end if
    __ref_n = __ref_n + $01
end macro

macro _ref_signature? ref?*, n?*
    _find_byte __ref_open,  ref, n, $28, $00
    _find_byte __ref_close, ref, n, $29, $00
    _slice __ref_rv, __ref_rl, ref, n, __ref_close + $01, n
    __ref_n = $00
    __ref_nreg = $00
    _walk_descs ref, n, __ref_open + $01, __ref_close, _sig_param
end macro

; Whether the return type is wide, which decides between
; move-result and move-result-wide at a call site.
macro _ref_ret_wide? result?*
    if (__ref_rl = $01) & ((__ref_rv = $4A) | (__ref_rv = $44))
        result = $01
    else
        result = $00
    end if
end macro
; A descriptor contributes one character to the shorty, and the one
; rule worth remembering is that an array is L whatever it is an
; array of — [I and [[Ljava/lang/String; are both L.  Written in one
; place because the prototype pool and the reference resolver both
; need it, and a shorty that disagrees with its own parameters is
; rejected by the verifier with nothing useful to say.
macro _shorty_append? descriptor?*
    __shortyc = (descriptor) and $FF
    if __shortyc = $5B
        __shortyc = $4C
    end if
    __shortyv = __shortyv or (__shortyc shl (__shortyl * $08))
    __shortyl = __shortyl + $01
end macro


macro _ref_shorty?
    __shortyv = __ref_rv and $FF
    if __shortyv = $5B
        __shortyv = $4C
    end if
    __shortyl = $01
    repeat __ref_n j:$00
        _shorty_append __ref_pv_#j
    end repeat
end macro

; A method is added the first time something names it, and what
; the key packs is kept apart as well.  Whether it is a member of a
; class, and its flags and code, are for its declaration to say.
macro _intern_method? cls?*, nm?*, proto?*
    __im_n = __method_n
    _intern __method, add, ((cls) shl $40) or ((nm) shl $20) or (proto)
    if __method_n > __im_n
        repeat 1, k:__method_found
            __mcls_#k = cls
            __mnm_#k = nm
            __mpr_#k = proto
            __mmember_#k = $00
            __mflags_#k = $00
            __mcode_#k = $00
            __mbody_#k = $00
        end repeat
    end if
end macro

; What a method reference contributes while the file is collected:
; its class, name, return and parameter types, its shorty, its
; prototype and the method itself, each interned where it is first
; met.  Indices here are the order things were met in; the ranks
; come once everything is in, in _dex_settle, and _intern_method
; answers in __method_found which method this was.
macro _use_method_ref? ref?*, n?*
    _split_method_ref ref, n
    _slice __ref_v, __ref_l, ref, n, $00, __ref_arrow
    _use_type __ref_v, __ref_l
    __um_cls = __type_found
    _slice __ref_v, __ref_l, ref, n, __ref_arrow + $02, __ref_open
    _intern __str, add, __ref_v, __ref_l
    __um_nm = __str_found
    _ref_signature ref, n
    _use_type __ref_rv, __ref_rl
    __um_ret = __type_found
    repeat __ref_n j:$00
        _use_type __ref_pv_#j, __ref_pl_#j
        __ref_p_#j = __type_found
    end repeat
    _ref_shorty
    _intern __str, add, __shortyv, __shortyl
    _intern_proto __um_ret, __ref_n, __str_found
    _intern_method __um_cls, __um_nm, __proto_found
end macro

; ── Resolving a field reference ───────────────────────────
;
;      iget v0, p0, "Lapp/hello/HelloWorld;->count:I"
;
;  Split on -> and the colon after it, then the class, the name and
;  the type give the field.

; Split a reference into __ref_arrow and __ref_colon.
macro _split_field_ref? ref?*, n?*
    _find_byte __ref_arrow, ref, n, $2D, $00
    if __ref_arrow < $00
        err 'a field reference reads Lclass;->name:type'
    end if
    _find_byte __ref_colon, ref, n, $3A, __ref_arrow
    if __ref_colon < $00
        err 'a field reference reads Lclass;->name:type'
    end if
end macro

; What a field reference contributes while the file is collected:
; its class, name and type, in __uf_cls, __uf_nm and __uf_ty for
; deffield, which is the one that keeps them.
macro _use_field_ref_types? ref?*, n?*
    _split_field_ref ref, n
    _slice __ref_v, __ref_l, ref, n, $00, __ref_arrow
    _use_type __ref_v, __ref_l
    __uf_cls = __type_found
    _slice __ref_v, __ref_l, ref, n, __ref_arrow + $02, __ref_colon
    _intern __str, add, __ref_v, __ref_l
    __uf_nm = __str_found
    _slice __ref_v, __ref_l, ref, n, __ref_colon + $01, n
    _use_type __ref_v, __ref_l
    __uf_ty = __type_found
end macro
