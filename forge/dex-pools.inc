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
;      collect what the file names   →  _collect
;      order the keys               →  _order
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
;  The string pool is not here: it lives with the walk that fills
;  it.  It is ordered by the same _order, with UTF-16 code points
;  compared where these compare numbers.
; ══════════════════════════════════════════════════════════

; ── Sorted pools ─────────────────────────────────────────
;
;  type_ids, field_ids and method_ids all sort by keys that are
;  plain numbers by the time they are emitted.  Composite keys pack
;  into one integer, major part first, so one comparison covers
;  every component.
;
;  Each pool is declared in a macro, because the list gets expanded
;  once to collect the keys and once per position to emit the entry
;  that belongs there.  Declaration order is free.

; One sort for every pool, the strings too: binary insertion over
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

; Contribute a descriptor: its text in the string phase, the type
; it names in the type phase.  A type joins the pool the first
; time a reference names it.
macro _use_type? v?*, l?*
    _chk_descriptor v, l
    if __sphase = $01
        _intern __str, add, v, l
    else if __deftype_mode = $02
        _lookup_string __ut_s, v, l
        _intern __type, add, __ut_s
    end if
end macro

; ── Type list ────────────────────────────────────────────
;
;      macro _types
;          deftype _activity
;          deftype _I
;      end macro
;
;      type_ids:
;          emit_type_ids _types
;
;  Sorted by the index of the descriptor string.

__deftype_mode   = $00      ; 0 emit, 2 collect
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
    __emitting = $00
    __type_n = $00
    _collect __deftype, $02
    __emitting = $01
    _order __type_v_, _key_less, __trank_, __tord_, __type_n, \
        'two types name the same descriptor'
    ; Nothing declares a type by name any more, so the items are
    ; emitted straight from what was collected.
    repeat __type_n r:$00
        repeat 1 t:__tord_#r
            __tid_#t type_id_item __type_v_#t
        end repeat
    end repeat
end macro

; ── Field list ───────────────────────────────────────────
;
;      macro _fields
;          deffield _f_count, _hello, _I, _count
;      end macro
;
;      field_ids:
;          emit_field_ids _fields
;
;  deffield names the field, then its class, its type and its name,
;  each by the base the type or string was declared under.
;  Sorted by class, then name, then type.

__deffield_mode   = $00
__deffield_index  = $00
__deffield_count  = $00

macro deffield? ref?*, flags?
    if (__sphase = $01) | (__deftype_mode = $02)
        _ref_pair ref
        _use_field_ref_types __rf_v, __rf_l
    else if __deffield_mode = $04
        __deffield_index = __deffield_index + $01
    else if __deffield_mode = $02
        _ref_pair ref
        _resolve_field_parts __rf_v, __rf_l
        repeat 1 k:__deffield_index
            match , flags
                __fmember_#k = $00
                __fflags_#k = $00
            else
                __fmember_#k = $01
                __fflags_#k = flags
            end match
            __fcls_#k = __ref_class
            __fnm_#k  = __ref_name
            __fty_#k  = __ref_type
            __fkey_#k = (__ref_class shl $40) or (__ref_name shl $20) or __ref_type
        end repeat
        __deffield_index = __deffield_index + $01
    end if
end macro

; Whether the file has any fields decides whether it carries a
; field_ids section at all, and the header needs to know before
; anything is emitted.  Counting them takes no pools.
macro count_fields?
    __deffield_index = $00
    _collect __deffield, $04
    __deffield_count = __deffield_index
end macro

macro emit_field_ids?
    __emitting = $00
    __deffield_index = $00
    _collect __deffield, $02
    __deffield_count = __deffield_index
    _order __fkey_, _key_less, __frank_, __ford_, __deffield_count, \
        'two fields share a class, a name and a type'
    ; Everything the item needs was recorded while collecting, so the
    ; items come straight out of that in rank order — the same as the
    ; type and method pools do it.  Reaching them by walking the lists
    ; again, once per field, is what this used to do.
    __emitting = $01
    repeat __deffield_count r:$00
        repeat 1 t:__ford_#r
            __fid_#t field_id_item __fcls_#t, __fty_#t, __fnm_#t
        end repeat
    end repeat
end macro

; ── Method list ──────────────────────────────────────────
;
;      macro _methods
;          defmethod _act_init_m, _activity, _void, _init
;      end macro
;
;      method_ids:
;          emit_method_ids _methods
;
;  defmethod names the method, then its class, its prototype and
;  its name, each by the base it was declared under.
;  Sorted by class, then name, then prototype.

__defmethod_mode   = $00
__defmethod_count  = $00

; A method the pool should carry that nothing calls.
; A method with no code of its own: one the pool should carry that
; nothing calls, or a native one, whose body is in the library.
macro defmethod? ref?*, flags?
    ; Phase two only binds what phase one interned, so a declaration
    ; has nothing to say during it.
    if __sphase <> $02
        _ref_pair ref
        _use_method_ref __rf_v, __rf_l
        if __defmethod_mode = $02
            match , flags
            else
                repeat 1 k:__mfound
                    __mmember_#k = $01
                    __mflags_#k = flags
                    __mcode_#k = $00
                end repeat
            end match
        end if
    end if
end macro

; ── Prototype list ───────────────────────────────────────
;
;      macro _protos
;          defproto _void,    _V,  _V,       <>
;          defproto _bundle,  _VL, _V,       <_bundle>
;          defproto _I_str,   _LI, _string,  <_I>
;      end macro
;
;      proto_ids:
;          emit_proto_ids _protos
;      data:
;          data_type_lists:
;              emit_proto_type_lists _protos
;
;  defproto names the prototype, its shorty string, its return type
;  and its parameter types, each by the base it was declared under.
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

__defproto_mode   = $00     ; 0 emit ids, 1 emit type lists, 2 collect
__defproto_index  = $00
__defproto_count  = $00
__defproto_maxpar = $00

macro defproto? base?*, return?*, params?
    if (__sphase = $01) | (__deftype_mode = $02)
        _use_type return, lengthof return
        iterate p?, params
            _use_type p, lengthof p
        end iterate
        if __sphase = $01
            _shorty_of return, params
            _intern __str, add, __shortyv, __shortyl
        end if
    else if __defproto_mode = $02
        _lookup_string __pt_s, return, lengthof return
        _lookup_type __pt_r, __pt_s
        __ref_n = $00
        iterate p?, params
            _lookup_string __pt_s, p, lengthof p
            _lookup_type __pt_t, __pt_s
            repeat 1 j:__ref_n
                __ref_p_#j = __pt_t
            end repeat
            __ref_n = __ref_n + $01
        end iterate
        _shorty_of return, params
        _lookup_string __pt_sh, __shortyv, __shortyl
        _intern_proto __pt_r, __ref_n, __pt_sh
        repeat 1 i:__defproto_index
            __pdecl_#i = __pfound
        end repeat
        __defproto_index = __defproto_index + $01
    else if __defproto_mode = $03
        repeat 1 i:__defproto_index
            repeat 1 k:__pdecl_#i
                base#_proto = __prank_#k
                base#_tl = __ptlname_#k
            end repeat
        end repeat
        __defproto_index = __defproto_index + $01
    end if
end macro

macro emit_proto_ids?
    __emitting = $00
    __defproto_count = $00
    __defproto_maxpar = $00
    __defproto_index = $00
    _collect __defproto, $02

    repeat __defproto_count k:$00
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

    _order __pkey_, _key_less, __prank_, __pord_, __defproto_count, \
        'two prototypes share a return type and parameter list'

    repeat __defproto_count k:$00
        __pbest = -$01
        repeat __defproto_count j:$00
            if __psig_#j = __psig_#k
                if (__pbest = -$01) | (__prank_#j < __pbestrank)
                    __pbest = j
                    __pbestrank = __prank_#j
                end if
            end if
        end repeat
        __powner_#k = __pbest
    end repeat

    ; Give the prototypes that were declared outright their names.
    __defproto_index = $00
    _collect __defproto, $03
    __emitting = $01

    ; Nothing declares most of these by name, so they are emitted
    ; straight from what was collected.
    repeat __defproto_count r:$00
        repeat 1 t:__pord_#r
            __pid_#t proto_id_item __pshorty_#t, __pret_#t, __ptlname_#t
        end repeat
    end repeat
end macro

; Lists come out in sorted order, each emitted once by the
; prototype that owns it.
macro emit_proto_type_lists?
    repeat __defproto_count r:$00
        repeat 1 k:__pord_#r
            if __pcnt_#k = $00
                __ptlname_#k = $00
            else if __powner_#k = k
                align $04
                __ptl_#k:
                __ptlname_#k = __ptl_#k
                __type_list_count? = __type_list_count? + $01
                uint __pcnt_#k
                repeat __pcnt_#k j:$00
                    ushort __pp_#k#_#j
                end repeat
            else
                repeat 1 o:__powner_#k
                    __ptlname_#k = __ptl_#o
                end repeat
            end if
        end repeat
    end repeat
end macro
