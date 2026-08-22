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
;    • what each kind of reference contributes, which differs by
;      phase: during collection it feeds the pools, and afterwards it
;      answers with indices.
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
;  Looking up, not registering: the pools are settled by the time
;  any code is assembled, so a descriptor naming something that was
;  never declared is an error rather than a new entry.  Registering
;  from the point of use would mean walking the method bodies
;  during the collect phase, which is a larger change.

; The one scan of the string pool.  A miss answers -1; _lookup_string
; below is this with the miss made fatal.
calminstruction _find_string? result*, value*, len*
    local e, n, sym, v, found, val, ln
    compute val, value
    compute ln, len
    ; a bare global reads in this instruction's own namespace, so
    ; name it the same way the entries are named
    arrange sym, =__snum
    compute n, sym
    compute e, 0
    compute found, -1
  scan:
    check e < n
    jno done
    arrange sym, =__strl_#e
    compute v, sym
    check v = ln
    jno step
    arrange sym, =__strv_#e
    compute v, sym
    check v = val
    jno step
    arrange sym, =__srank_#e
    compute found, sym
    jump done
  step:
    compute e, e + 1
    jump scan
  done:
    arrange sym, result
    publish sym, found
end calminstruction

; The pools are settled by the time anything looks a string up, so a
; miss is a mistake rather than a new entry.
macro _lookup_string? result?*, value?*, len?*
    _find_string result, value, len
    if result < $00
        err 'no string in the pool matches this text'
    end if
end macro

; ── Finding an entry in a ranked pool ─────────────────────
;
;  type_ids, field_ids and method_ids are each a table of keys with a
;  table of ranks beside it, and finding an entry is the same scan in
;  all three: walk the keys, answer the rank that sits at the same
;  index.  The tables differ only in their names, and a name is a
;  token, so it can be an argument — `keys#=_#k` composes __tkey_3
;  the same way the specialised versions used to spell it out.
;
;  A key is one integer even when it names three things: the callers
;  below pack class, name and prototype into one, major part first,
;  so a single comparison covers the whole reference.
calminstruction _find_ranked? result*, keys*, ranks*, count*, key*
    local k, n, sym, v, found, want
    compute want, key
    arrange sym, count
    compute n, sym
    compute k, 0
    compute found, -1
  scan:
    check k < n
    jno done
    arrange sym, keys#=_#k
    compute v, sym
    check v = want
    jno step
    arrange sym, ranks#=_#k
    compute found, sym
    jump done
  step:
    compute k, k + 1
    jump scan
  done:
    arrange sym, result
    publish sym, found
end calminstruction

macro _lookup_type? result?*, sidx?*
    _find_ranked result, __tkey, __trank, __deftype_count, sidx
    if result < $00
        err 'no type is declared with this descriptor'
    end if
end macro

; Resolve a descriptor to its type index in __resolved_type.
macro _resolve_type? descriptor?*&
    _chk_descriptor descriptor, lengthof descriptor
    _lookup_string __resolved_string, descriptor, lengthof descriptor
    _lookup_type __resolved_type, __resolved_string
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

calminstruction _lookup_proto? result*, ret*, n*
    local k, cnt, sym, v, w, found, r, nn, j
    compute r, ret
    compute nn, n
    arrange sym, =__defproto_count
    compute cnt, sym
    compute k, 0
    compute found, -1
  scan:
    check k < cnt
    jno done
    arrange sym, =__pret_#k
    compute v, sym
    check v = r
    jno step
    arrange sym, =__pcnt_#k
    compute v, sym
    check v = nn
    jno step
    compute j, 0
  pscan:
    check j < nn
    jno hit
    arrange sym, =__pp_#k#=_#j
    compute v, sym
    arrange sym, =__ref_p_#j
    compute w, sym
    check v = w
    jno step
    compute j, j + 1
    jump pscan
  hit:
    arrange sym, =__prank_#k
    compute found, sym
    jump done
  step:
    compute k, k + 1
    jump scan
  done:
    check found >= 0
    jyes ok
    err "no prototype is declared with this signature"
  ok:
    arrange sym, result
    publish sym, found
end calminstruction

macro _lookup_method? result?*, cls?*, nm?*, proto?*
    _find_ranked result, __mkey, __mrank, __defmethod_count, \
        ((cls) shl $40) or ((nm) shl $20) or (proto)
    if result < $00
        err 'no method is declared with this reference'
    end if
end macro

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

; The descriptor between from and to, as a type index.  A reference
; is nothing but a handful of these read out of one string.
macro _type_at? result?*, ref?*, n?*, from?*, to?*
    _slice __ref_v, __ref_l, ref, n, from, to
    _chk_descriptor __ref_v, __ref_l
    _lookup_string __ref_s, __ref_v, __ref_l
    _lookup_type result, __ref_s
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

; During the collect phase the pools do not exist yet, so a
; reference contributes its pieces to the string pool and nothing
; more.  The reference itself is never interned, only the class
; descriptor, the name and the parameter and return descriptors.
macro _use_method_ref_types? ref?*, n?*
    _split_method_ref ref, n
    _slice __ref_v, __ref_l, ref, n, $00, __ref_arrow
    _use_type __ref_v, __ref_l
    if __sphase = $01
        _slice __ref_v, __ref_l, ref, n, __ref_arrow + $02, __ref_open
        _intern __ref_v, __ref_l
    end if
    _slice __ref_v, __ref_l, ref, n, __ref_close + $01, n
    _use_type __ref_v, __ref_l
    _walk_descs ref, n, __ref_open + $01, __ref_close, _use_type
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

; The shorty a reference implies, interned in the string phase so a
; prototype registered from it has one to point at.
macro _use_method_ref_shorty? ref?*, n?*
    _ref_signature ref, n
    _ref_shorty
    _intern __shortyv, __shortyl
end macro

macro _use_method_ref_proto? ref?*, n?*
    _ref_signature ref, n
    _lookup_string __pt_s, __ref_rv, __ref_rl
    _lookup_type __pt_r, __pt_s
    repeat __ref_n j:$00
        _lookup_string __pt_s, __ref_pv_#j, __ref_pl_#j
        _lookup_type __pt_t, __pt_s
        __ref_p_#j = __pt_t
    end repeat
    _ref_shorty
    _lookup_string __pt_sh, __shortyv, __shortyl
    _intern_proto __pt_r, __ref_n, __pt_sh
end macro

; A method is added the first time something names it.
calminstruction _intern_method? cls*, nm*, proto*
    local k, cnt, sym, v, key, c, n2, p2, z
    compute c, cls
    compute n2, nm
    compute p2, proto
    compute z, 0
    compute key, (c shl $40) or (n2 shl $20) or p2
    arrange sym, =__defmethod_count
    compute cnt, sym
    compute k, 0
  scan:
    check k < cnt
    jno add
    arrange sym, =__mkey_#k
    compute v, sym
    check v = key
    jyes done
    compute k, k + 1
    jump scan
  add:
    arrange sym, =__mkey_#cnt
    publish sym, key
    arrange sym, =__mcls_#cnt
    publish sym, c
    arrange sym, =__mnm_#cnt
    publish sym, n2
    arrange sym, =__mpr_#cnt
    publish sym, p2
    arrange sym, =__mmember_#cnt
    publish sym, z
    arrange sym, =__mflags_#cnt
    publish sym, z
    arrange sym, =__mcode_#cnt
    publish sym, z
    compute k, cnt
    compute cnt, cnt + 1
    arrange sym, =__defmethod_count
    publish sym, cnt
  done:
    arrange sym, =__mfound
    publish sym, k
end calminstruction

; What a method reference contributes, whichever pool is asking.
macro _use_method_ref? v?*, l?*
    if __sphase = $01
        _use_method_ref_types v, l
        _use_method_ref_shorty v, l
    else if __deftype_mode = $02
        _use_method_ref_types v, l
    else if __defproto_mode = $02
        _use_method_ref_proto v, l
    else if __defmethod_mode = $02
        _resolve_method_parts v, l
        _intern_method __ref_class, __ref_name, __ref_proto
    end if
end macro

; Resolve a reference into its class, name and prototype.
; One parameter of a reference being resolved: its type index, kept
; where _lookup_proto expects to find it.
macro _rmp_param? v?*, l?*
    _chk_descriptor v, l
    _lookup_string __ref_s, v, l
    _lookup_type __ref_t, __ref_s
    repeat 1 j:__ref_n
        __ref_p_#j = __ref_t
    end repeat
    __ref_n = __ref_n + $01
end macro

macro _resolve_method_parts? ref?*, n?*
    _split_method_ref ref, n

    _type_at __ref_class, ref, n, $00, __ref_arrow

    _slice __ref_v, __ref_l, ref, n, __ref_arrow + $02, __ref_open
    _lookup_string __ref_name, __ref_v, __ref_l

    _type_at __ref_ret, ref, n, __ref_close + $01, n

    __ref_n = $00
    _walk_descs ref, n, __ref_open + $01, __ref_close, _rmp_param

    _lookup_proto __ref_proto, __ref_ret, __ref_n
end macro

macro _resolve_method? ref?*, n?*
    _resolve_method_parts ref, n
    _lookup_method __ref_method, __ref_class, __ref_name, __ref_proto
end macro

; ── Resolving a field reference ───────────────────────────
;
;      iget v0, p0, "Lapp/hello/HelloWorld;->count:I"
;
;  Split on -> and the colon after it, then the class, the name and
;  the type give the field.

macro _lookup_field? result?*, cls?*, nm?*, type?*
    _find_ranked result, __fkey, __frank, __deffield_count, \
        ((cls) shl $40) or ((nm) shl $20) or (type)
    if result < $00
        err 'no field is declared with this reference'
    end if
end macro

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

; Resolve a reference into its class, name and type.
macro _resolve_field_parts? ref?*, n?*
    _split_field_ref ref, n
    _type_at __ref_class, ref, n, $00, __ref_arrow
    _slice __ref_v, __ref_l, ref, n, __ref_arrow + $02, __ref_colon
    _lookup_string __ref_name, __ref_v, __ref_l
    _type_at __ref_type, ref, n, __ref_colon + $01, n
end macro

macro _resolve_field? ref?*, n?*
    _resolve_field_parts ref, n
    _lookup_field __ref_field, __ref_class, __ref_name, __ref_type
end macro

macro _use_field_ref_types? ref?*, n?*
    _split_field_ref ref, n
    _slice __ref_v, __ref_l, ref, n, $00, __ref_arrow
    _use_type __ref_v, __ref_l
    if __sphase = $01
        _slice __ref_v, __ref_l, ref, n, __ref_arrow + $02, __ref_colon
        _intern __ref_v, __ref_l
    end if
    _slice __ref_v, __ref_l, ref, n, __ref_colon + $01, n
    _use_type __ref_v, __ref_l
end macro
