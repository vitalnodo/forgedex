; ══════════════════════════════════════════════════════════
;  dex-pools.inc — the four pools that are sorted by a key
;
;  The format requires type_ids, proto_ids, field_ids and method_ids
;  to come out sorted, and it does not say sorted by what you might
;  guess: types by the index of their descriptor string, fields and
;  methods by class then name then type, prototypes by return type
;  then parameters.
;
;  All four are built the same way, and seeing that is the point of
;  putting them together:
;
;      collect what the file names   →  _dex_collect, one walk
;      order the keys               →  _dex_settle, with _order
;      emit the items in rank order
;
;  A composite key is packed into one integer — class, name and type
;  shifted into their own halves — so a single numeric comparison
;  covers a whole reference and the sort needs no special cases.
;
;  The four emitters below read almost alike, and it is worth saying
;  where they are not, because a shared one would have hidden it.
;  They differ in how an entry gets its slot:
;
;    types       interned by descriptor — naming the same type twice
;                lands on the same slot, silently, which is what lets
;                every instruction name its types outright;
;    methods     interned the same way, by class, name and prototype;
;    fields      appended in declaration order, not interned.  Two
;                fields that agree on all three are a mistake rather
;                than one field, and _order is where that is
;                caught;
;    prototypes  interned, and additionally bound to any name a
;                declaration gave them.
;
;  So the shape is shared and the slot allocation is not.
;
;  The string pool's declarations are in dex-strings.inc, but it is
;  collected and ordered here with the rest, by the same _order,
;  with UTF-16 code units compared where these compare numbers.
; ══════════════════════════════════════════════════════════

; ── Sorted pools ─────────────────────────────────────────
;
;  type_ids, field_ids and method_ids all sort by keys that are
;  plain numbers by the time they are emitted.  Composite keys pack
;  into one integer, major part first, so one comparison covers
;  every component.
;
;  Declaration order is free: what comes out depends on the keys.

; Contribute a descriptor: its text to the string pool and the type
; it names to the type pool, which holds the string's place until
; the strings are sorted.  A type joins the pool the first time a
; reference names it, and __type_found says which it is.
macro _use_type? v?*, l?*
    _chk_descriptor v, l
    _intern __str, add, v, l
    _intern __type, add, __str_found
end macro

; ── Type list ────────────────────────────────────────────
;
;      macro _types
;          deftype "Ljava/lang/Object;"
;      end macro
;
;  Most types arrive through a reference — a class, a superclass, a
;  parameter, new_instance — and deftype is for one that nothing
;  names.  Sorted by the index of the descriptor string.

__type_n         = $00

; A type descriptor is a primitive letter, or L followed by a name
; and a semicolon, or any of those behind one or more [.  Nothing
; checked this, and a mistyped one assembles into a dex that only
; fails when the class is loaded on the device.
;
; Reading a descriptor byte by byte is what parsing a full
; smali-style reference would be built from, so it is a CALM
; instruction.

calminstruction _chk_descriptor? v*, l*
    local i, b, e
    compute i, 0
  arr:
    check i < l
    jno bad
    compute b, (v shr (i * 8)) and $FF
    check b = $5B
    jno head
    compute i, i + 1
    jump arr
  head:
    check b = $4C
    jyes ref
    ; a primitive has to be the whole of what is left
    check i + 1 = l
    jno bad
    check b = $56
    jyes ok
    check b = $5A
    jyes ok
    check b = $42
    jyes ok
    check b = $53
    jyes ok
    check b = $43
    jyes ok
    check b = $49
    jyes ok
    check b = $4A
    jyes ok
    check b = $46
    jyes ok
    check b = $44
    jyes ok
    jump bad
  ref:
    ; L, at least one character of name, then the semicolon
    check i + 2 < l
    jno bad
    compute e, (v shr ((l - 1) * 8)) and $FF
    check e = $3B
    jno bad
    jump ok
  bad:
    err "not a type descriptor"
  ok:
end calminstruction

; A type that no reference names, but that the pool should carry
; anyway.  Everything else arrives through a reference.
macro deftype? descriptor?*&
    _use_type descriptor, lengthof descriptor
end macro

macro emit_type_ids?
    repeat __type_n r:$00
        repeat 1 t:__tord_#r
            __tid_#t type_id_item __type_v_#t
        end repeat
    end repeat
end macro

; ── Field list ───────────────────────────────────────────
;
;      deffield "->count:I", ACC_PUBLIC
;      deffield "Landroid/graphics/Bitmap$Config;->ARGB_8888:Landroid/graphics/Bitmap$Config;"
;
;  A field is named by its reference, "->" meaning the class declared
;  last.  With access flags it is a member of that class; without,
;  it is one the code reaches in some other class.  Sorted by class,
;  then name, then type.

__deffield_index  = $00
__field_n         = $00

macro deffield? ref?*, flags?
    _ref_pair ref
    _use_field_ref_types __rf_v, __rf_l
    repeat 1 k:__deffield_index
        match , flags
            __fmember_#k = $00
            __fflags_#k = $00
        else
            __fmember_#k = $01
            __fflags_#k = flags
        end match
        __fcls_#k = __uf_cls
        __fnm_#k  = __uf_nm
        __fty_#k  = __uf_ty
    end repeat
    __deffield_index = __deffield_index + $01
end macro

macro emit_field_ids?
    repeat __field_n r:$00
        repeat 1 t:__ford_#r
            __fid_#t field_id_item __fcls_#t, __fty_#t, __fnm_#t
        end repeat
    end repeat
end macro

; ── Method list ──────────────────────────────────────────
;
;      defmethod "->answer()Ljava/lang/String;", ACC_PUBLIC or ACC_NATIVE
;      defmethod "Ljava/lang/Integer;->toString(I)Ljava/lang/String;"
;
;  Most methods arrive through a reference — an invoke, or a method
;  with its body — and defmethod is for one with no body here: a
;  native one, given flags, or one the pool should carry anyway.
;  Sorted by class, then name, then prototype.

__method_n  = $00

; A method the pool should carry that nothing calls.
; A method with no code of its own: one the pool should carry that
; nothing calls, or a native one, whose body is in the library.
macro defmethod? ref?*, flags?
    _ref_pair ref
    _use_method_ref __rf_v, __rf_l
    match , flags
    else
        repeat 1 k:__method_found
            __mmember_#k = $01
            __mflags_#k = flags
            __mcode_#k = $00
        end repeat
    end match
end macro

; ── Prototype list ───────────────────────────────────────
;
;      macro _protos
;          defproto _oncl, "V", <"Landroid/view/View$OnClickListener;">
;          defproto _I,    "I", <>
;      end macro
;
;  Most prototypes arrive through a method reference.  defproto is
;  for one wanted by name: it gives _oncl_proto, the prototype's
;  index, and _oncl_tl, its parameter list, which a class can point
;  its interfaces at.
;
;  proto_ids sort by return type and then by parameter list compared
;  element by element, so the parameters have to be readable at sort
;  time.  Spelling them here rather than pointing at a type_list in
;  the data section is what makes that possible, and it lets the
;  parameter lists be generated instead of written out: prototypes
;  taking the same parameters share one list, and a prototype taking
;  none gets no list at all.
;
;  A generated list is named after the prototype that owns it, so
;  <_oncl> parameters land in _oncl_tl, which a class can point its
;  interfaces at.
;
;  Keys pack the return type above the parameters, each parameter
;  held as its index plus one so that an absent parameter reads as
;  zero and a shorter list sorts before a longer one sharing its
;  start.

__defproto_index  = $00
__proto_n  = $00
__defproto_maxpar = $00

macro defproto? base?*, return?*, params?
    _use_type return, lengthof return
    __dp_ret = __type_found
    __ref_n = $00
    iterate p?, params
        _use_type p, lengthof p
        repeat 1 j:__ref_n
            __ref_p_#j = __type_found
        end repeat
        __ref_n = __ref_n + $01
    end iterate
    _shorty_of return, params
    _intern __str, add, __shortyv, __shortyl
    _intern_proto __dp_ret, __ref_n, __str_found
    ;  The name is given once the ranks are in, by _dex_settle.
    repeat 1 i:__defproto_index
        __pdecl_#i = __proto_found
        __pdecl_name_#i equ base
    end repeat
    __defproto_index = __defproto_index + $01
end macro

macro emit_proto_ids?
    repeat __proto_n r:$00
        repeat 1 t:__pord_#r
            __pid_#t proto_id_item __pshorty_#t, __pret_#t, __ptlname_#t
        end repeat
    end repeat
end macro

; Lists come out in sorted order, each emitted once by the
; prototype that owns it.
macro emit_proto_type_lists?
    repeat __proto_n r:$00
        repeat 1 k:__pord_#r
            if __pcnt_#k = $00
                __ptlname_#k = $00
            else if __powner_#k = k
                align $04
                __ptl_#k:
                __ptlname_#k = __ptl_#k
                _type_list __pcnt_#k
                repeat __pcnt_#k j:$00
                    _type_list_type __pp_#k#_#j
                end repeat
            else
                repeat 1 o:__powner_#k
                    __ptlname_#k = __ptl_#o
                end repeat
            end if
        end repeat
    end repeat
end macro

; ── Collecting, once ──────────────────────────────────────
;
;  The lists are walked once, and everything they name goes into
;  its pool where it is first met: a string, a type as the string
;  it names, a prototype as its types, a method as its class, name
;  and prototype, a field, a class.  An index is the order things
;  were met in, and it does not move for the rest of the pass, so
;  each pool can point into the ones below it already.
;
;  Then the pools are put in order from the bottom up, and each one
;  read through the ranks of the ones under it — the strings first,
;  then the types by their strings' ranks, the prototypes by their
;  types', and the fields and methods by all of those.  The code is
;  assembled in the same walk, with ranks it is given a pass early.

macro _dex_collect?
    __emitting = $00
    __str_n = $00
    __type_n = $00
    __proto_n = $00
    __defproto_maxpar = $00
    __method_n = $00
    __defstr_index = $00
    __defproto_index = $00
    __deffield_index = $00
    __fref_n = $00
    __fdecl_n = $00
    ;  The interfaces a class lists are collected too: a type that
    ;  nothing else names would otherwise not be in the pool when
    ;  its list comes to be written.  They go first, so that a name
    ;  they set — `_hello_ifaces = _oncl_tl` — is already set when a
    ;  class reads it, rather than being read ahead of a second
    ;  setting, which fasmg will not do.
    virtual at $00
        match i, __dex_interfaces
            i
        end match
        _walk_lists
    end virtual
    __class_count = __defclass_index
    _dex_settle
    __emitting = $01
end macro

macro _dex_settle?
    ;  Interning means equal strings never reach the sort, so the
    ;  check it ends with can only catch a slip in the insertion.
    _order __str, _str_less, __srank_, __ord_, __str_n, \
        'string pool came out unsorted'
    repeat __defstr_index k:$00
        repeat 1 e:__dstr_slot_#k
            match b, __dstr_name_#k
                b#_string = __srank_#e
            end match
        end repeat
    end repeat

    ;  A type is held as its string until the strings have ranks.
    repeat __type_n t:$00
        repeat 1 s:__type_v_#t
            __type_v_#t = __srank_#s
        end repeat
    end repeat
    _order __type_v_, _key_less, __trank_, __tord_, __type_n, \
        'two types name the same descriptor'

    ;  A prototype's parts become ranks, and so does the key it is
    ;  found by, for the code to find it with what it resolves.
    repeat __proto_n k:$00
        repeat 1 t:__pret_#k
            __pret_#k = __trank_#t
        end repeat
        repeat __pcnt_#k j:$00
            repeat 1 t:__pp_#k#_#j
                __pp_#k#_#j = __trank_#t
            end repeat
        end repeat
        repeat 1 s:__pshorty_#k
            __pshorty_#k = __srank_#s
        end repeat
        __pk = __pret_#k
        repeat __pcnt_#k j:$00
            __pk = __pk or ((__pp_#k#_#j + $01) shl ($20 * (j + $01)))
        end repeat
        __proto_v_#k = __pk
    end repeat
    repeat __proto_n k:$00
        __pkey_#k = (__pret_#k) shl ($20 * __defproto_maxpar)
        __psig_#k = $00
        repeat __defproto_maxpar j:$00
            if j < __pcnt_#k
                __pkey_#k = __pkey_#k or \
                    ((__pp_#k#_#j + $01) shl ($20 * (__defproto_maxpar - $01 - j)))
                __psig_#k = __psig_#k or ((__pp_#k#_#j + $01) shl ($20 * j))
            end if
        end repeat
    end repeat
    _order __pkey_, _key_less, __prank_, __pord_, __proto_n, \
        'two prototypes share a return type and parameter list'
    ;  Prototypes taking the same parameters share one list, owned
    ;  by the first of them in rank order.
    repeat __proto_n k:$00
        __pbest = -$01
        repeat __proto_n j:$00
            if __psig_#j = __psig_#k
                if (__pbest = -$01) | (__prank_#j < __pbestrank)
                    __pbest = j
                    __pbestrank = __prank_#j
                end if
            end if
        end repeat
        __powner_#k = __pbest
    end repeat
    repeat __defproto_index i:$00
        repeat 1 k:__pdecl_#i
            match b, __pdecl_name_#i
                b#_proto = __prank_#k
                b#_tl = __ptlname_#k
            end match
        end repeat
    end repeat

    __field_n = __deffield_index
    ;  The declared fields by their parts as met, for the references
    ;  to be matched against once the fields have ranks.
    repeat __field_n k:$00
        _intern __fdecl, add, (__fcls_#k shl $40) or (__fnm_#k shl $20) or __fty_#k,, k
    end repeat
    repeat __field_n k:$00
        repeat 1 c:__fcls_#k, n:__fnm_#k, t:__fty_#k
            __fcls_#k = __trank_#c
            __fnm_#k  = __srank_#n
            __fty_#k  = __trank_#t
        end repeat
        __field_v_#k = (__fcls_#k shl $40) or (__fnm_#k shl $20) or __fty_#k
    end repeat
    _order __field_v_, _key_less, __frank_, __ford_, __field_n, \
        'two fields share a class, a name and a type'
    repeat __fref_n r:$00
        _intern __fdecl, find, __fref_v_#r
        if __fdecl_found < $00
            __frefrank_#r = -$01
        else
            repeat 1 e:__fdecl_found
                repeat 1 k:__fdecl_i_#e
                    __frefrank_#r = __frank_#k
                end repeat
            end repeat
        end if
    end repeat

    repeat __method_n k:$00
        repeat 1 c:__mcls_#k, n:__mnm_#k, p:__mpr_#k
            __mcls_#k = __trank_#c
            __mnm_#k  = __srank_#n
            __mpr_#k  = __prank_#p
        end repeat
        __method_v_#k = (__mcls_#k shl $40) or (__mnm_#k shl $20) or __mpr_#k
    end repeat
    _order __method_v_, _key_less, __mrank_, __mord_, __method_n, \
        'two methods share a class, a name and a prototype'

    repeat __class_count k:$00
        repeat 1 c:__dccls_#k, s:__dcsup_#k
            __dccls_#k = __trank_#c
            __dcsup_#k = __trank_#s
        end repeat
    end repeat
end macro
