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
    repeat __deffield_count k:$00
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
    repeat __defmethod_count k:$00
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
; were written in.
macro _cd_emit_fields? want_static?*
    local d, diff
    _enc_group_start
    repeat __deffield_count r:$00
        repeat __deffield_count k:$00
            if (__frank_#k = r) & __fmember_#k & (__fcls_#k = __cd_cls)
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
    repeat __defmethod_count r:$00
        repeat __defmethod_count k:$00
            if (__mrank_#k = r) & __mmember_#k & (__mcls_#k = __cd_cls)
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
    _find_string __cd_init, "<init>", $06
    _find_string __cd_clinit, "<clinit>", $08
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

macro emit_class_defs?
    __class_defs_phase = $01
    __defclass_index = $00
    _walk_lists
    __class_count = __defclass_index
    __class_defs_phase = $00
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
    if __class_defs_phase
        _lookup_string __dc_s, descriptor, lengthof descriptor
        _lookup_type __dc_cls, __dc_s
        _lookup_string __dc_s, super, lengthof super
        _lookup_type __dc_sup, __dc_s
        match , ifaces
            __dc_if = $00
        else
            __dc_if = ifaces
        end match
        repeat 1 k:__defclass_index
            __class_def_#k class_def_item __dc_cls, flags, __dc_sup, __dc_if, \
                NO_INDEX, $00, __class_data_#k, $00
        end repeat
    else
        _use_type descriptor, lengthof descriptor
        _use_type super, lengthof super
    end if
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
        __type_list_count? = __type_list_count? + $01
        uint n
        iterate t?, types
            _lookup_string __dtl_s, t, lengthof t
            _lookup_type __dtl_t, __dtl_s
            ushort __dtl_t
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
    if __code_phase
        _resolve_method_parts __m_v, __m_l
        _intern_method __ref_class, __ref_name, __ref_proto
        __m_k = __mfound
    else
        _use_method_ref __m_v, __m_l
        if __defmethod_mode = $02
            __m_k = __mfound
            repeat 1 k:__m_k
                __mmember_#k = $01
                __mflags_#k = flags
                __mcode_#k = __code_#k
            end repeat
        end if
    end if
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
    virtual at $00
    if __code_phase
        repeat 1 k:__m_k
            __code_#k#_insns::
        end repeat
    else
        repeat 1 c:__collect_seq
            __collect_anchor_#c::
        end repeat
        __collect_seq = __collect_seq + $01
    end if
end macro

macro end?.method?
    end virtual
    if __code_phase
        if __max_reg >= __m_regs
            err 'method uses a register at or above the frame size'
        end if
        repeat 1 k:__m_k
            __code_#k code_item __m_regs, __m_ins, __max_outs, $00, $00, $FFFF, __code_#k#_insns
        end repeat
        align $04
    end if
end macro

macro emit_method_ids?
    __emitting = $00
    __defmethod_count = $00
    _collect __defmethod, $02
    _rank_pool __mkey_, __mrank_, __mord_, __defmethod_count, \
        'two methods share a class, a name and a prototype'
    ; No naming pass here, unlike the prototypes: a method is reached
    ; by its reference rather than by a name given to a declaration,
    ; so there is nothing left to bind once the ranks are in.
    __emitting = $01
    repeat __defmethod_count r:$00
        repeat 1 t:__mord_#r
            __mid_#t method_id_item __mcls_#t, __mpr_#t, __mnm_#t
        end repeat
    end repeat
end macro

; A prototype is added the first time something names its
; signature.  Matching is on the return type and the parameter
; types, so two references to the same signature give one entry.
calminstruction _intern_proto? ret*, n*, shorty*
    local k, cnt, sym, v, w, j, r, nn, sh
    compute r, ret
    compute nn, n
    compute sh, shorty
    arrange sym, =__defproto_count
    compute cnt, sym
    compute k, 0
  scan:
    check k < cnt
    jno add
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
    jno done
    arrange sym, =__pp_#k#=_#j
    compute v, sym
    arrange sym, =__ref_p_#j
    compute w, sym
    check v = w
    jno step
    compute j, j + 1
    jump pscan
  step:
    compute k, k + 1
    jump scan
  add:
    arrange sym, =__pret_#cnt
    publish sym, r
    arrange sym, =__pcnt_#cnt
    publish sym, nn
    arrange sym, =__pshorty_#cnt
    publish sym, sh
    compute j, 0
  copy:
    check j < nn
    jno bump
    arrange sym, =__ref_p_#j
    compute v, sym
    arrange sym, =__pp_#cnt#=_#j
    publish sym, v
    compute j, j + 1
    jump copy
  bump:
    compute k, cnt
    compute cnt, cnt + 1
    arrange sym, =__defproto_count
    publish sym, cnt
    arrange sym, =__defproto_maxpar
    compute v, sym
    check nn > v
    jno done
    arrange sym, =__defproto_maxpar
    publish sym, nn
  done:
    arrange sym, =__pfound
    publish sym, k
end calminstruction

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

; ── Method code helper ───────────────────────────────────
;
;  The frame is written the way smali writes it: say how many
;  locals the body needs and how many registers come in, and the
;  total is worked out here.
;
;      dex_method _hello_oncreate_code, locals 4, ins 2
;          invoke_super _act_oncr_m, p0, p1
;          return_void
;      end dex_method
;
;  Incoming arguments sit at the top of the frame, so their numbers
;  depend on the total and shift whenever the body needs another
;  local.  Refer to them as p0, p1 ... and that stops mattering;
;  p0 is `this` for an instance method.  Locals stay v0, v1 ...
;
;  A total register count may still be given directly, in which
;  case the caller works out where the arguments landed:
;
;      dex_method _hello_init_code, $01, ins 1
;
;  outs_size is derived from the widest invoke in the body, and a
;  register at or above the total is an error.
;
;  Branch targets are labels, not offsets.  A local label attaches
;  to the method it sits in, so each method has its own namespace
;  and two methods may both use .loop:
;
;      dex_method _hello_run_code, locals 2, ins 0
;      .loop:
;          add_int_lit8 v0, v0, -1
;          if_eqz       v0, .done
;          goto         .loop
;      .done:
;          return_void
;      end dex_method

macro dex_method? _name?*, _frame?*, _ins?*
    __dex_method_name equ _name
    match =ins n, _ins
        __dex_method_ins = n
    else
        __dex_method_ins = _ins
    end match
    match =locals n, _frame
        __dex_method_regs = n + __dex_method_ins
    else
        __dex_method_regs = _frame
    end match
    if __dex_method_ins > __dex_method_regs
        err 'method takes in more registers than the frame holds'
    end if
    ; Drop the previous method's argument names first, so that
    ; naming an argument this method does not take is an undefined
    ; symbol rather than a leftover number.
    repeat $100 i:$00
        if defined p#i
            restore p#i
        end if
    end repeat
    repeat __dex_method_ins i:$00
        p#i = __dex_method_regs - __dex_method_ins + i
    end repeat
    __max_reg = -$01
    __max_outs = $00
    virtual at $00
    if __code_phase
        _name#_insns::
    else
        ; A throwaway anchor, so that local labels in the body have
        ; something of their own to hang on during the extra passes
        ; the collect phase makes over the code.
        repeat 1 c:__collect_seq
            __collect_anchor_#c::
        end repeat
        __collect_seq = __collect_seq + $01
    end if
end macro

macro end?.dex_method?
    end virtual
    if __code_phase
        if __max_reg >= __dex_method_regs
            err 'method uses a register at or above registers_size'
        end if
        match name, __dex_method_name
            name code_item __dex_method_regs, __dex_method_ins, __max_outs, $00, $00, $FFFF, name#_insns
        end match
    end if
    restore __dex_method_name
    align $04
end macro

__collect_seq = $00

; The code is handed to emit_string_ids as one more list so that the
; literals and descriptors written into it reach the pool, and then
; assembled here for real.
macro emit_code?
    __code_phase = $01
    _walk_lists
    __code_phase = $00
end macro
