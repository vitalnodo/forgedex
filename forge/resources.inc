; ══════════════════════════════════════════════════════════
;  resources.inc — a resource table, declared rather than built
;
;  A resource id is not a name the platform looks up.  It is a
;  number, and the number is decided by where the thing sits:
;
;      $7F      02      0000
;      package  type    entry
;
;  The type is the position of its name in the type pool counting
;  from one; the entry is its position within that type counting
;  from zero.  So the order of a declaration fixes every number in
;  it, and aapt writes R.java for exactly that reason — to stop the
;  table and the code that reads it from disagreeing.
;
;  Nothing here generates Java, so the declaration plays that part:
;  the constants, both name pools, the values pool and every entry
;  all come out of one walk, and no file holds a number of its own.
;
;  An example writes this and nothing else:
;
;      macro each_type? handler?*
;          ;        type name,  what a value is,  entries,  configs
;          handler "drawable",  VALUE_FILE,  _icons,   _icon_cfgs
;          handler "string",    VALUE_TEXT,  _texts,   _text_cfgs
;          handler "id",        VALUE_NONE,  _ids,     _id_cfgs
;      end macro
;
;      macro _icons? handler?*
;          handler R_DRAWABLE_ICON, "icon"
;      end macro
;
;      macro _icon_cfgs? handler?*
;          handler cfg_any, "res/drawable/icon.png"
;      end macro
;
;      resource_ids                       ; defines R_*, gathers pools
;      resource_table "app.hello.demo"    ; emits the table
;
;  A type holds entries, and a type holds configurations.  An entry
;  is a name and a number; a configuration is one value for each of
;  those entries, in the same order.  That is how R.layout.screen
;  can be one number with two files behind it, or R.string.greeting
;  one number with two texts — the platform picks when the resource
;  is asked for, by comparing the device against each config.
; ══════════════════════════════════════════════════════════

if ~ definite __forge_resources_included
__forge_resources_included := $01

; ── What a value is ───────────────────────────────────────
;
;  A whole type has one kind, the way it does in Android: every
;  entry of `color` is a colour, every entry of `layout` is a file.
;
;  VALUE_FILE and VALUE_TEXT are the same code — both put a string
;  in the values pool and write its index.  They differ only in
;  what the string means, a path or the text itself, which is why
;  localisation costs this file nothing.

VALUE_NONE?  := $00     ; holds nothing; an id exists to be a number
VALUE_FILE?  := $01     ; a path, in the values pool
VALUE_TEXT?  := $02     ; the text itself, in the same pool
VALUE_COLOR? := $03     ; $AARRGGBB, inline
VALUE_INT?   := $04     ; inline
VALUE_BOOL?  := $05     ; $FFFFFFFF is true, inline

;  Only FILE and NONE are used by anything here — themes has an
;  icon, a layout and an id.  The rest were checked against a table
;  built by hand and are correct, but nothing in this repository
;  exercises them, so treat them as untested until something does.

macro _res_vtype? result?*, kind?*
    if      kind = VALUE_FILE  | kind = VALUE_TEXT
        result = TYPE_STRING
    else if kind = VALUE_COLOR
        result = TYPE_INT_COLOR_ARGB8
    else if kind = VALUE_INT
        result = TYPE_INT_DEC
    else
        result = TYPE_INT_BOOLEAN
    end if
end macro

macro _res_pools? result?*, kind?*
    ;  fasmg compares in `if`, not as an operator that yields a
    ;  value, so this cannot be written as one expression.
    if kind = VALUE_FILE | kind = VALUE_TEXT
        result = $01
    else
        result = $00
    end if
end macro

; ── Configurations, as macros ─────────────────────────────
;
;  A configuration is written as a macro so a declaration names it
;  rather than repeating its qualifiers.  These two are the common
;  ones; an example defines its own the same way:
;
;      macro cfg_uk
;          config "uk"
;      end macro

macro cfg_any
    config
end macro

; Unused here for the same reason; the 8 is the API the qualifier
; needs, which is what aapt writes too.
macro cfg_night
    config "", UI_MODE_NIGHT_YES, 8
end macro

; ── The numbers ───────────────────────────────────────────

PACKAGE_ID? := $7F

macro resource_ids
    __r_tid = $00

    macro _r_type? ty?*, kind?*, entries?*, configs?*
        __r_tid = __r_tid + $01
        __r_eid = $00
        entries _r_entry
    end macro

    macro _r_entry? name?*, key?*
        name := (PACKAGE_ID shl $18) or (__r_tid shl $10) or __r_eid
        __r_eid = __r_eid + $01
    end macro

    each_type _r_type
    purge _r_type, _r_entry

    TYPE_COUNT? := __r_tid
    _res_gather_pools
end macro

; ── The three string pools ────────────────────────────────
;
;  A pool is one variadic call, so the names are collected into a
;  list first.  This is fasmg's idiom for that: match what is there
;  already and re-`equ` it with one more on the end.
;
;  Such a list stays a single token until something splits it, so a
;  pool has to be handed it through `match` — passed directly it
;  would see one string named RES_TYPE_NAMES.

RES_TYPE_NAMES equ
RES_KEY_NAMES  equ
RES_VALUE_STRINGS equ

macro _res_gather? into?*, value?*
    match any, into
        into equ any, value
    else
        into equ value
    end match
end macro

macro _res_gather_pools
    macro _r_ctype? ty?*, kind?*, entries?*, configs?*
        _res_gather RES_TYPE_NAMES, ty
        entries _r_ckey
        _res_pools __r_pools, kind
        if __r_pools
            configs _r_cvalues
        end if
    end macro

    macro _r_ckey? name?*, key?*
        _res_gather RES_KEY_NAMES, key
    end macro

    ;  Every value any configuration names goes into the pool, so a
    ;  second language adds strings there and nothing else.
    macro _r_cvalues? cfg?*, values?&
        iterate v, values
            _res_gather RES_VALUE_STRINGS, v
        end iterate
    end macro

    each_type _r_ctype
    purge _r_ctype, _r_ckey, _r_cvalues
end macro

; ── The table ─────────────────────────────────────────────

;  Every entry is the same sixteen bytes — eight of header, eight
;  of value — which is why the offset table is arithmetic.
RES_ENTRY_SIZE? := $10

macro _res_entry? key?*, vtype?*, value?*
    ushort $08
    ushort $00
    uint   key
    ushort $08
    ubyte  $00
    ubyte  vtype
    uint   value
end macro

;  The package name is 128 UTF-16 units, zero padded.  One byte at
;  a time out of a virtual block, because a string cannot be
;  indexed directly.
macro _res_package_name? name?*
    virtual at $00
        db name
        __rn_len = $ - $$
    end virtual
    repeat __rn_len, i:$00
        virtual at $00
            db name
            load __rn_c : $01 from i
        end virtual
        ushort __rn_c
    end repeat
    repeat 128 - __rn_len
        ushort $00
    end repeat
end macro

macro resource_table? pkg_name?*
    chunk restable, RES_TABLE
        uint $01                          ; one package
    restable_header_end:

        match names, RES_VALUE_STRINGS
            string_pool resvalues, names
        end match

        chunk respkg, RES_TABLE_PACKAGE
            uint  PACKAGE_ID
        respkg_name:
            _res_package_name pkg_name
            uint  (restypes_start - respkg_start)
            uint  $00
            uint  (reskeys_start - respkg_start)
            uint  $00
            uint  $00
        respkg_header_end:

            match names, RES_TYPE_NAMES
                string_pool restypes, names
            end match
            match names, RES_KEY_NAMES
                string_pool reskeys, names
            end match

            ;  Three counters run across the whole table: the type
            ;  id, where this type's keys start in the key pool, and
            ;  how far the values pool has been used.  The entry
            ;  number is a position within its type, so it is not
            ;  one of them.
            __t_tid  = $00
            __t_key  = $00
            __t_val  = $00

            each_type _res_emit_type
    respkg_end:
    restable_end:
end macro

macro _res_emit_type? ty?*, kind?*, entries?*, configs?*
    __t_tid = __t_tid + $01

    __t_count = $00
    macro _t_count? name?*, key?*
        __t_count = __t_count + $01
    end macro
    entries _t_count
    purge _t_count

    _res_vtype __t_vtype, kind
    _res_pools __t_pools, kind

    repeat 1, t:__t_tid, n:__t_count
        chunk resspec_#t, RES_TABLE_TYPE_SPEC
            ubyte  t
            ubyte  $00
            ushort $00
            uint   n
        resspec_#t#_header_end:
            repeat n
                uint $00                  ; which configs vary: unused
            end repeat
        resspec_#t#_end:

        __t_cfg = $00
        macro _t_config? cfg?*, values?&
            __t_cfg = __t_cfg + $01
            repeat 1, c:__t_cfg
                chunk restype_#t#_#c, RES_TABLE_TYPE
                    ubyte  t
                    ubyte  $00
                    ushort $00
                    uint   n
                    uint   (restype_#t#_#c#_entries - restype_#t#_#c#_start)
                    cfg
                restype_#t#_#c#_header_end:
                    repeat n, i:$00
                        uint i * RES_ENTRY_SIZE
                    end repeat
                restype_#t#_#c#_entries:
                    __t_i = $00
                    iterate v, values
                        if __t_pools
                            _res_entry (__t_key + __t_i), __t_vtype, __t_val
                            __t_val = __t_val + $01
                        else
                            _res_entry (__t_key + __t_i), __t_vtype, v
                        end if
                        __t_i = __t_i + $01
                    end iterate
                    ;  No values at all is a type of id resources:
                    ;  each entry is a false boolean and means only
                    ;  its own number.
                    match , values
                        repeat n, i:$00
                            _res_entry (__t_key + i), __t_vtype, $00
                        end repeat
                    end match
                restype_#t#_#c#_end:
            end repeat
        end macro

        configs _t_config
        purge _t_config
    end repeat

    __t_key = __t_key + __t_count
end macro

end if
