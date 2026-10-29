; ══════════════════════════════════════════════════════════
;  dex-class.inc — declaring a class, and the code in its methods
;
;  What an example actually writes: defclass, deffield, defmethod,
;  and method … end method with instructions between.
;
;  class_data_item is the fiddly part.  It holds four lists — static
;  fields, instance fields, direct methods, virtual methods — and
;  each stores its members as a *difference* from the one before,
;  restarting at every list.  So the sizes and the deltas both fall
;  out of counting, and nothing here is written by hand.
;
;  A method's frame is written the way smali writes it: say how many
;  locals, and the parameters are named p0, p1 … from the signature.
;  outs_size is worked out from the invokes in the body, and a
;  register outside the frame is an error rather than a corrupt dex.
;  The body is assembled into a virtual block first, so that `$` is
;  the address of the instruction and a branch can name a label.
; ══════════════════════════════════════════════════════════

; ── Class data ───────────────────────────────────────────
;
;      defclassdata _hello_class_data, "Lapp/hello/HelloWorld;"
;
;  The members are the fields and methods declared with access
;  flags, and which of them belong here is decided by the class in
;  their own reference.  Sizes, the split between the two field
;  lists and the two method lists, and the ascending order the
;  format wants are all worked out from that.
;
;  A method is direct when it is a constructor, static or private,
;  and virtual otherwise.

macro _cd_is_direct? result?*, k?*
    result = $00
    if (__mflags_#k and (ACC_STATIC or ACC_PRIVATE))
        result = $01
    end if
    if __cd_init >= $00
        if __mnm_#k = __cd_init
            result = $01
        end if
    end if
    if __cd_clinit >= $00
        if __mnm_#k = __cd_clinit
            result = $01
        end if
    end if
end macro

macro _cd_count_fields? result?*, want_static?*
    local d
    result = $00
    repeat __field_n k:$00
        if __fmember_#k & (__fcls_#k = __cd_cls)
            d = $00
            if (__fflags_#k and ACC_STATIC)
                d = $01
            end if
            if d = want_static
                result = result + $01
            end if
        end if
    end repeat
end macro

macro _cd_count_methods? result?*, want_direct?*
    local d
    result = $00
    repeat __method_n k:$00
        if __mmember_#k & (__mcls_#k = __cd_cls)
            _cd_is_direct d, k
            if d = want_direct
                result = result + $01
            end if
        end if
    end repeat
end macro

; Walking the ranks rather than the declarations puts the entries
; in the ascending order the format asks for, whatever order they
; were written in.  The entry at each rank is read from the order
; table rather than searched for: the search made the class data
; quadratic in the number of members, and it was most of the time
; a build took.
macro _cd_emit_fields? want_static?*
    local d, diff
    _enc_group_start
    repeat __field_n r:$00
        repeat 1 k:__ford_#r
            if __fmember_#k & (__fcls_#k = __cd_cls)
                d = $00
                if (__fflags_#k and ACC_STATIC)
                    d = $01
                end if
                if d = want_static
                    _enc_idx_diff diff, r, 'fields'
                    leb128 diff
                    leb128 __fflags_#k
                end if
            end if
        end repeat
    end repeat
end macro

macro _cd_emit_methods? want_direct?*
    local d, diff
    _enc_group_start
    repeat __method_n r:$00
        repeat 1 k:__mord_#r
            if __mmember_#k & (__mcls_#k = __cd_cls)
                _cd_is_direct d, k
                if d = want_direct
                    _enc_idx_diff diff, r, 'methods'
                    leb128 diff
                    leb128 __mflags_#k
                    leb128 __mcode_#k
                end if
            end if
        end repeat
    end repeat
end macro

struc defclassdata? k?*
    local nsf, nif, ndm, nvm
    _lookup_string __cd_s, __clsv_#k, __clsl_#k
    _lookup_type __cd_cls, __cd_s
    _find_ranked __cd_init, __str, __srank_, "<init>", $06
    _find_ranked __cd_clinit, __str, __srank_, "<clinit>", $08
    _cd_count_fields  nsf, $01
    _cd_count_fields  nif, $00
    _cd_count_methods ndm, $01
    _cd_count_methods nvm, $00
    label .
    leb128 nsf
    leb128 nif
    leb128 ndm
    leb128 nvm
    _cd_emit_fields  $01
    _cd_emit_fields  $00
    _cd_emit_methods $01
    _cd_emit_methods $00
end struc

; ── Class declaration ────────────────────────────────────
;
;      macro _class
;          defclass _hello_class, "Lapp/hello/HelloWorld;", ACC_PUBLIC, \
;              "Landroid/app/Activity;", _hello_ifaces, _hello_class_data
;      end macro
;
;  Named by descriptor like everything else, so the class and its
;  superclass reach the type pool the same way.

; The classes come out by the index of their type, not in the order
; the lists declare them, except that a class this file also defines
; as a superclass or an interface goes first, which the format
; requires.  Each pick is the smallest type among those whose own
; supertypes are already out.  The class data follows the same order.
macro emit_class_defs?
    repeat __class_count k:$00
        __cplaced_#k = $00
    end repeat
    repeat __class_count r:$00
        __cbest = -$01
        repeat __class_count k:$00
            if __cplaced_#k = $00
                __cready = $01
                repeat __class_count j:$00
                    if __cplaced_#j = $00
                        _class_needs __cneeds, k, __dccls_#j
                        if __cneeds
                            __cready = $00
                        end if
                    end if
                end repeat
                if __cready & ((__cbest < $00) | (__dccls_#k < __cbestkey))
                    __cbest = k
                    __cbestkey = __dccls_#k
                end if
            end if
        end repeat
        if __cbest < $00
            err 'classes in this file extend or implement each other in a circle'
        else
            repeat 1 k:__cbest
                __cplaced_#k = $01
                __cord_#r = k
                __class_def_#k class_def_item __dccls_#k, __dcflags_#k, __dcsup_#k, \
                    __dcif_#k, NO_INDEX, $00, __class_data_#k, $00
            end repeat
        end if
    end repeat
end macro

; Whether class k has to wait for the class whose type is t: t is
; its superclass, or one of the interfaces it lists, found in the
; table of type lists by where its list is.
macro _class_needs? result?*, k?*, t?*
    result = $00
    if __dcsup_#k = t
        result = $01
    end if
    repeat __tl_total, i:$00
        if __tl_at_#i = __dcif_#k
            repeat __tl_n_#i, j:$00
                if __tl_t_#i#_#j = t
                    result = $01
                end if
            end repeat
        end if
    end repeat
end macro

macro defclass? descriptor?*, flags?*, super?*, ifaces?
    ; The class being declared, for the members that follow it, and
    ; kept per class so that a file may hold several.
    __cls_v = descriptor
    __cls_l = lengthof descriptor
    repeat 1 k:__defclass_index
        __clsv_#k = descriptor
        __clsl_#k = lengthof descriptor
    end repeat
    ;  Kept as types met, made ranks by _dex_settle, and written by
    ;  emit_class_defs once every class is known.
    _use_type descriptor, lengthof descriptor
    __dc_cls = __type_found
    _use_type super, lengthof super
    repeat 1 k:__defclass_index
        __dccls_#k = __dc_cls
        __dcflags_#k = flags
        __dcsup_#k = __type_found
        match , ifaces
            __dcif_#k = $00
        else
            __dcif_#k = ifaces
        end match
    end repeat
    __defclass_index = __defclass_index + $01
end macro

; A standalone list of types, for the interfaces a class implements.
struc deftypelist? types?*&
    local n
    if __emitting
        n = $00
        iterate t?, types
            n = n + $01
        end iterate
        align $04
        label .
        _type_list n
        iterate t?, types
            _lookup_string __dtl_s, t, lengthof t
            _lookup_type __dtl_t, __dtl_s
            _type_list_type __dtl_t
        end iterate
    else
        iterate t?, types
            _use_type t, lengthof t
        end iterate
    end if
end struc

; ── A method of the class, with its code ─────────────────
;
;      method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 2
;          invoke_super "Landroid/app/Activity;->onCreate(...)V", p0, p1
;          return_void
;      end method
;
;  ins is the signature's own: one register per parameter — two for
;  a long or a double — and one more for this unless the method is
;  static.  p0, p1 ... name registers rather than parameters, as
;  smali does, so a double parameter answers to two of them.

macro method? ref?*, flags?*, frame?*
    _ref_pair ref
    __m_v = __rf_v
    __m_l = __rf_l
    _ref_signature __m_v, __m_l
    __m_ins = __ref_nreg
    if ~ (flags and ACC_STATIC)
        __m_ins = __m_ins + $01
    end if
    match =locals n, frame
        __m_regs = n + __m_ins
    else
        __m_regs = frame
    end match
    if __m_ins > __m_regs
        err 'method takes in more registers than the frame holds'
    end if
    _use_method_ref __m_v, __m_l
    __m_k = __method_found
    repeat 1 k:__m_k
        __mmember_#k = $01
        __mflags_#k = flags
        __mcode_#k = __code_#k
        __mbody_#k = $01
    end repeat
    repeat $100 i:$00
        if defined p#i
            restore p#i
        end if
    end repeat
    repeat __m_ins i:$00
        p#i = __m_regs - __m_ins + i
    end repeat
    __max_reg = -$01
    __max_outs = $00
    ;  The body goes into an area of its own, once, with its final
    ;  indices; emit_code copies it out in method order.
    virtual at $00
    repeat 1 k:__m_k
        __code_#k#_insns::
    end repeat
end macro

macro end?.method?
    end virtual
    if __max_reg >= __m_regs
        err 'method uses a register at or above the frame size'
    end if
    repeat 1 k:__m_k
        __cregs_#k = __m_regs
        __cins_#k  = __m_ins
        __couts_#k = __max_outs
    end repeat
end macro

macro emit_method_ids?
    repeat __method_n r:$00
        repeat 1 t:__mord_#r
            __mid_#t method_id_item __mcls_#t, __mpr_#t, __mnm_#t
        end repeat
    end repeat
end macro

; A prototype is its return type and its parameter types, and those
; pack into one number: the return type, then each parameter type
; plus one, 32 bits apiece.  The plus one keeps a shorter list from
; matching a longer one that ends in type 0.  The parameters are
; read from __ref_p_0 onwards, where every caller has put them.
macro _proto_key? ret?*, n?*
    __pk = ret
    repeat n, j:$00
        __pk = __pk or ((__ref_p_#j + $01) shl ($20 * (j + $01)))
    end repeat
end macro

; A prototype is added the first time something names its
; signature, so two references to the same signature give one
; entry.  Its parts are kept beside it for the sort and the items.
macro _intern_proto? ret?*, n?*, shorty?*
    _proto_key ret, n
    __ip_n = __proto_n
    _intern __proto, add, __pk
    if __proto_n > __ip_n
        repeat 1, k:__proto_found
            __pret_#k = ret
            __pcnt_#k = n
            __pshorty_#k = shorty
            repeat n, j:$00
                __pp_#k#_#j = __ref_p_#j
            end repeat
        end repeat
        if n > __defproto_maxpar
            __defproto_maxpar = n
        end if
    end if
end macro

; The shorty of a signature: the return character then one per
; parameter, an L standing for any reference or array.
macro _shorty_of? retv?*, params?&
    __shortyv = (retv) and $FF
    if __shortyv = $5B
        __shortyv = $4C
    end if
    __shortyl = $01
    iterate p?, params
        _shorty_append p
    end iterate
end macro

; The items come out in method order, not in the order the bodies
; were met, so that moving a method in the source moves nothing in
; the file.  The bodies were assembled while the lists were
; collected, each in an area of its own and independent of where it
; lands, so this is only copying.
macro emit_code?
    repeat __method_n r:$00
        repeat 1 k:__mord_#r
            if __mbody_#k
                __code_#k code_item __cregs_#k, __cins_#k, __couts_#k, $00, $00, $FFFF, __code_#k#_insns
                align $04
            end if
        end repeat
    end repeat
end macro
