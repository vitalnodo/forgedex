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
;  Written for every class by dex_file; nothing declares it.  The
;  members are the fields and methods declared with access
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
    _lookup_type __cd_cls, __clsv_#k, __clsl_#k
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
;          defclass "Lapp/hello/HelloWorld;", ACC_PUBLIC, \
;              "Landroid/app/Activity;", _hello_ifaces
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
            _lookup_type __dtl_t, t, lengthof t
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
    __m_pl_from = __pl_seq
    repeat 1 k:__m_k
        __ctry_n_#k = $00
    end repeat
    ;  The body goes into an area of its own, once, with its final
    ;  indices; emit_code copies it out in method order.
    virtual at $00
    repeat 1 k:__m_k
        __code_#k#_insns::
    end repeat
end macro

macro end?.method?
    _pl_layout
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
                _try_group k
                __code_#k code_item __cregs_#k, __cins_#k, __couts_#k, __tg_n, $00, $FFFF, __code_#k#_insns
                _try_write k
                align $04
            end if
        end repeat
    end repeat
end macro

; The payloads of the method being closed, at the end of its body,
; each four-byte aligned — nop, which is two zero bytes, fills.
macro _pl_layout?
    repeat __pl_seq - __m_pl_from, q:$00
        repeat 1 s:__m_pl_from + q
            align $04
          __payload_#s:
            if __pl_kind_#s = $01
                dw $0100, __pl_n_#s
                dd (__pl_key_#s) and $FFFFFFFF
                repeat __pl_n_#s j:$00
                    dd __pl_t_#s#_#j and $FFFFFFFF
                end repeat
            else if __pl_kind_#s = $02
                dw $0200, __pl_n_#s
                repeat __pl_n_#s r:$00
                    repeat 1 j:__pl_o_#s#_#r
                        dd __pl_k_#s#_#j and $FFFFFFFF
                    end repeat
                end repeat
                repeat __pl_n_#s r:$00
                    repeat 1 j:__pl_o_#s#_#r
                        dd __pl_t_#s#_#j and $FFFFFFFF
                    end repeat
                end repeat
            else
                dw $0300, __pl_w_#s
                dd __pl_n_#s
                repeat __pl_n_#s j:$00
                    emit __pl_w_#s : (__pl_d_#s#_#j) and (($01 shl (__pl_w_#s * $08)) - $01)
                end repeat
                align $02
            end if
        end repeat
    end repeat
end macro

; The catches of method k, as try items: one per range, the ranges in
; ascending order, each with its handlers in the order written.
; Ranges that are the same are one item; ranges that overlap without
; being the same would have to be cut where they cross, which is not
; done here, so they are an error.
macro _try_group? k?*
    __tg_n = $00
    repeat __ctry_n_#k i:$00
        __tg_at = -$01
        repeat __tg_n g:$00
            if (__tg_s_#g = __ctry_s_#k#_#i) & (__tg_e_#g = __ctry_e_#k#_#i)
                __tg_at = g
            end if
        end repeat
        if __tg_at < $00
            __tg_at = __tg_n
            repeat 1 g:__tg_at
                __tg_s_#g = __ctry_s_#k#_#i
                __tg_e_#g = __ctry_e_#k#_#i
                __tg_hn_#g = $00
                __tg_all_#g = -$01
            end repeat
            __tg_n = __tg_n + $01
        end if
        repeat 1 g:__tg_at
            if __ctry_t_#k#_#i = -$01
                if __tg_all_#g >= $00
                    err 'one try range has two catch-alls'
                end if
                __tg_all_#g = __ctry_h_#k#_#i
            else
                if __tg_all_#g >= $00
                    err 'a catch after the catch-all of its range is never reached'
                end if
                repeat 1 h:__tg_hn_#g
                    __tg_ht_#g#_#h = __ctry_t_#k#_#i
                    __tg_ha_#g#_#h = __ctry_h_#k#_#i
                end repeat
                __tg_hn_#g = __tg_hn_#g + $01
            end if
        end repeat
    end repeat
    _order __tg_s_, _key_less, __tg_r_, __tg_o_, __tg_n, \
        'two try ranges of one method start together but end apart'
    if __tg_n > $01
        repeat __tg_n - $01, r:$00, r1:$01
            repeat 1 a:__tg_o_#r, b:__tg_o_#r1
                if __tg_e_#a > __tg_s_#b
                    err 'try ranges of one method overlap; nested ranges are not supported yet'
                end if
            end repeat
        end repeat
    end if
end macro

; After the instructions: two bytes of padding when they are an odd
; number of units, the try items, then the handler lists they point
; into, each at an offset from the start of the list.
macro _try_write? k?*
    if __tg_n
        if ($ - __code_#k#.insns) and $02
            dw $0000
        end if
        repeat __tg_n r:$00
            repeat 1 g:__tg_o_#r
                uint   __tg_s_#g
                ushort __tg_e_#g - __tg_s_#g
                ushort __handler_#k#_#g - __handlers_#k
            end repeat
        end repeat
      __handlers_#k:
        leb128 __tg_n
        repeat __tg_n r:$00
            repeat 1 g:__tg_o_#r
              __handler_#k#_#g:
                if __tg_all_#g >= $00
                    sleb128 -__tg_hn_#g
                else
                    sleb128 __tg_hn_#g
                end if
                repeat __tg_hn_#g h:$00
                    leb128 __tg_ht_#g#_#h, __tg_ha_#g#_#h
                end repeat
                if __tg_all_#g >= $00
                    leb128 __tg_all_#g
                end if
            end repeat
        end repeat
    end if
end macro
