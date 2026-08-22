; ══════════════════════════════════════════════════════════
;  forgeaxml.inc — AndroidManifest.xml, hand-forged
;
;  An APK carries its manifest as binary AXML, not as text.  The
;  format is chunked the same way resources.arsc is, and holds:
;
;      RES_XML
;      ├── string pool
;      ├── resource map        one id per attribute name, in order
;      ├── START_NAMESPACE
;      │   └── elements, nested
;      └── END_NAMESPACE
;
;  The resource map is the part that matters and the part every
;  simpler tool gets wrong.  An attribute reaches the platform only
;  if its name sits in that map with the framework's id for it;
;  a name without an id is read, then ignored.  That is why the
;  first strings in the pool are exactly the attribute names, in
;  the same order as the map — the two are parallel arrays.
;
;  Written as:
;
;      macro _manifest
;          element "manifest"
;              attr_plain "package", "app.hello.dtmf"
;              element "uses-sdk"
;                  attr_int "minSdkVersion", 21
;              end element
;          end element
;      end macro
;
;      axml_file _manifest
;
;  Attributes are android: unless declared with attr_plain.  The
;  ids come from the table at the end of this file, which was read
;  out of framework-res.apk on a device rather than typed in.
; ══════════════════════════════════════════════════════════

if ~ definite __forgeaxml_included
__forgeaxml_included := $01

;  The chunk header layout and the value types are shared with
;  resources.arsc — it is the same format — so they come from
;  there rather than being spelled twice.
include 'forgeres.inc'

RES_XML?              := $0003
RES_XML_START_NS?     := $0100
RES_XML_END_NS?       := $0101
RES_XML_START_ELEMENT? := $0102
RES_XML_END_ELEMENT?  := $0103
RES_XML_RESOURCE_MAP? := $0180


;  A dimension carries its unit in the low four bits and the value
;  above them, shifted according to a radix in bits 4..5.  Whole
;  numbers need no fraction, so radix 0 and a plain shift by eight
;  covers everything a layout is written with.
DIM_PX?  := $00
DIM_DP?  := $01
DIM_SP?  := $02

AXML_NS? := "http://schemas.android.com/apk/res/android"

; ── The attribute table ───────────────────────────────────
;
;  Name to framework resource id.  An attribute that is not here
;  cannot be written, which is deliberate: the alternative is
;  emitting it with no id, where it would be silently dropped by
;  the platform instead of loudly by the assembler.

__axt_n = $00

macro defattr? name?*, id?*
    _ax_table_add name, id
end macro

calminstruction _ax_table_add? name*, id*
    local n, sym, v
    compute v, name
    compute n, __axt_n
    arrange sym, =__axt_v_#n
    publish sym, v
    compute v, id
    arrange sym, =__axt_i_#n
    publish sym, v
    compute n, n + 1
    arrange sym, =__axt_n
    publish sym, n
end calminstruction

; Answers into __ax_id, or -1 when the name is not in the table.
calminstruction _ax_table_find? name*
    local e, n, sym, v, val, found
    compute val, name
    compute n, __axt_n
    compute e, 0
    compute found, -1
  scan:
    check e < n
    jno done
    arrange sym, =__axt_v_#e
    compute v, sym
    check v = val
    jno step
    arrange sym, =__axt_i_#e
    compute found, sym
    jump done
  step:
    compute e, e + 1
    jump scan
  done:
    arrange sym, =__ax_id
    publish sym, found
end calminstruction

; ── The two string pools ──────────────────────────────────
;
;  Attribute names go in the first, everything else in the second,
;  and the pool written out is one after the other.  Keeping them
;  apart is what lets the resource map stay parallel to the front
;  of the pool.

__axr_n = $00               ; attribute names
__axo_n = $00               ; all other strings

calminstruction _ax_res? name*, id*
    local e, n, sym, v, val, found
    compute val, name
    compute n, __axr_n
    compute e, 0
    compute found, -1
  scan:
    check e < n
    jno add
    arrange sym, =__axr_v_#e
    compute v, sym
    check v = val
    jno step
    compute found, e
    jump done
  step:
    compute e, e + 1
    jump scan
  add:
    arrange sym, =__axr_v_#n
    publish sym, val
    compute v, id
    arrange sym, =__axr_i_#n
    publish sym, v
    compute found, n
    compute n, n + 1
    arrange sym, =__axr_n
    publish sym, n
  done:
    arrange sym, =__ax_found
    publish sym, found
end calminstruction

calminstruction _ax_str? value*
    local e, n, sym, v, val, found
    compute val, value
    compute n, __axo_n
    compute e, 0
    compute found, -1
  scan:
    check e < n
    jno add
    arrange sym, =__axo_v_#e
    compute v, sym
    check v = val
    jno step
    compute found, e
    jump done
  step:
    compute e, e + 1
    jump scan
  add:
    arrange sym, =__axo_v_#n
    publish sym, val
    compute found, n
    compute n, n + 1
    arrange sym, =__axo_n
    publish sym, n
  done:
    arrange sym, =__ax_found
    publish sym, found
end calminstruction

; An "other" string's place in the written pool is behind all the
; attribute names, so its index is only known once collecting is
; over.  These two resolve a name to its final index.

macro _ax_ires? value?*
    _ax_str value
    __ax_idx = __axr_n + __ax_found
end macro

macro _ax_ires_attr? name?*
    _ax_table_find name
    if __ax_id = -1
        err 'unknown android attribute: ', name, ' — add it to the table in forgeaxml.inc'
    end if
    ;  Ascending resource id, within an element, is not a matter of
    ;  taste.  The platform resolves an element's attributes with a
    ;  cursor that only walks forwards, so one written out of turn
    ;  is stepped over — read, and then not used.  aapt sorts; there
    ;  is nothing here to sort with, so it is the source that has to
    ;  be in order, and saying so is the whole point of checking.
    if __ax_last_id > __ax_id
        err 'attribute out of order: ', name, ' must come before the one written above it'
    end if
    __ax_last_id = __ax_id
    _ax_res name, __ax_id
    __ax_idx = __ax_found
end macro

; ── The description, walked twice ─────────────────────────
;
;  Phase 1 interns strings and counts each element's attributes;
;  phase 2 writes the chunks.  An element's attribute count has to
;  be in its header, ahead of the attributes themselves, and both
;  walks visit the same elements in the same order — so the count
;  found in the first is the one used in the second.

__ax_phase = $00
__ax_elem  = $00            ; element ordinal, in visiting order
__ax_depth = $00
__ax_last_id = $00          ; the id of the attribute written last

; An index has to become a repeat counter before # can paste it:
; the operator joins tokens, not the values behind them.
macro _ax_open
    repeat 1, d:__ax_depth
        __ax_stack_#d = __ax_elem
    end repeat
end macro

macro element? name?*
    ;  A fresh element starts the ordering over: the ids have to
    ;  ascend within an element, not across the file.
    __ax_last_id = $00
    if __ax_phase = $01
        ;  The raw place in the second pool, not the final index:
        ;  the first pool is still growing as attribute names are
        ;  met, so the offset to add is only settled once the walk
        ;  is over.
        _ax_str name
        ; remember which element is open at this depth, so that
        ; attr_* knows whose count to raise
        _ax_open
        repeat 1, e:__ax_elem
            __ax_attrs_#e = $00
            __ax_name_#e = __ax_found
        end repeat
        __ax_elem = __ax_elem + $01
        __ax_depth = __ax_depth + $01
    else if __ax_phase = $02
        _ax_open
        repeat 1, e:__ax_elem
            __ax_count_tmp = __ax_attrs_#e
            __ax_name_tmp  = __axr_n + __ax_name_#e
        end repeat
        __ax_elem = __ax_elem + $01
        __ax_depth = __ax_depth + $01

        ushort RES_XML_START_ELEMENT
        ushort $10                        ; header size
        uint   $10 + $14 + $14 * __ax_count_tmp
        uint   $01                        ; line number
        uint   $FFFFFFFF                  ; comment: none
        uint   $FFFFFFFF                  ; namespace of the element
        uint   __ax_name_tmp
        ushort $14                        ; where attributes start
        ushort $14                        ; size of one attribute
        ushort __ax_count_tmp
        ushort $00                        ; id attribute index
        ushort $00                        ; class attribute index
        ushort $00                        ; style attribute index
    end if
end macro

macro end?.element
    if __ax_phase = $01
        __ax_depth = __ax_depth - $01
    else if __ax_phase = $02
        __ax_depth = __ax_depth - $01
        repeat 1, d:__ax_depth
            __ax_elem_tmp = __ax_stack_#d
        end repeat
        repeat 1, e:__ax_elem_tmp
            __ax_name_tmp = __axr_n + __ax_name_#e
        end repeat

        ushort RES_XML_END_ELEMENT
        ushort $10
        uint   $18
        uint   $01
        uint   $FFFFFFFF
        uint   $FFFFFFFF
        uint   __ax_name_tmp
    end if
end macro

; ── Attributes ────────────────────────────────────────────

macro _ax_count
    repeat 1, d:(__ax_depth - $01)
        __ax_elem_tmp = __ax_stack_#d
    end repeat
    repeat 1, e:__ax_elem_tmp
        __ax_attrs_#e = __ax_attrs_#e + $01
    end repeat
end macro

;  ns, name, rawValue, then the value itself.
macro _ax_emit? nsidx?*, nameidx?*, raw?*, type?*, data?*
    uint   nsidx
    uint   nameidx
    uint   raw
    ushort $08                            ; size of a Res_value
    ubyte  $00                            ; res0
    ubyte  type
    ;  The field is four bytes whatever it holds — a size of -1
    ;  for match_parent as readily as a colour — so it is masked
    ;  rather than range-checked as an unsigned would be.
    uint   ((data) and $FFFFFFFF)
end macro

macro attr_str? name?*, value?*
    if __ax_phase = $01
        _ax_ires_attr name
        _ax_ires value
        _ax_count
    else if __ax_phase = $02
        _ax_ires_attr name
        __ax_n_tmp = __ax_idx
        _ax_ires value
        __ax_v_tmp = __ax_idx
        _ax_ires AXML_NS
        _ax_emit __ax_idx, __ax_n_tmp, __ax_v_tmp, TYPE_STRING, __ax_v_tmp
    end if
end macro

;  package= on <manifest> is the one attribute with no namespace
;  and no resource id.
macro attr_plain? name?*, value?*
    if __ax_phase = $01
        _ax_ires name
        _ax_ires value
        _ax_count
    else if __ax_phase = $02
        _ax_ires name
        __ax_n_tmp = __ax_idx
        _ax_ires value
        _ax_emit $FFFFFFFF, __ax_n_tmp, __ax_idx, TYPE_STRING, __ax_idx
    end if
end macro

macro _ax_typed? name?*, type?*, data?*
    if __ax_phase = $01
        _ax_ires_attr name
        _ax_count
    else if __ax_phase = $02
        _ax_ires_attr name
        __ax_n_tmp = __ax_idx
        _ax_ires AXML_NS
        _ax_emit __ax_idx, __ax_n_tmp, $FFFFFFFF, type, data
    end if
end macro

macro attr_int? name?*, value?*
    _ax_typed name, TYPE_INT_DEC, value
end macro

;  A boolean is an int whose true is every bit set.
macro attr_bool? name?*, value?*
    if value
        _ax_typed name, TYPE_INT_BOOLEAN, $FFFFFFFF
    else
        _ax_typed name, TYPE_INT_BOOLEAN, $00
    end if
end macro

;  A reference names a resource by id — @drawable/icon, once the
;  table in resources.arsc says which id that is.
macro attr_ref? name?*, value?*
    _ax_typed name, TYPE_REFERENCE, value
end macro

;  match_parent and wrap_content are not names in the file — they
;  are the integers -1 and -2, which is what the inflater compares
;  against.
MATCH_PARENT? := -1
WRAP_CONTENT? := -2

;  A float is carried as its own bits, so the assembler is asked
;  to lay one out and then to read the four bytes back.
macro attr_float? name?*, value?*
    virtual at $00
        dd value
        load __ax_fbits : $04 from $00
    end virtual
    _ax_typed name, TYPE_FLOAT, __ax_fbits
end macro

;  A size in density-independent pixels, sp for text.
macro attr_dp? name?*, value?*
    _ax_typed name, TYPE_DIMENSION, (value shl $08) or DIM_DP
end macro

macro attr_sp? name?*, value?*
    _ax_typed name, TYPE_DIMENSION, (value shl $08) or DIM_SP
end macro

;  A colour written straight into the attribute — $FF336699 — with
;  no entry in any resource table.
macro attr_color? name?*, value?*
    _ax_typed name, TYPE_INT_COLOR_ARGB8, value
end macro

;  style= is the odd one out: no android: prefix, and its value
;  points at an attribute of the theme rather than at a resource.
macro attr_style? value?*
    if __ax_phase = $01
        _ax_ires "style"
        _ax_count
    else if __ax_phase = $02
        _ax_ires "style"
        _ax_emit $FFFFFFFF, __ax_idx, $FFFFFFFF, TYPE_ATTRIBUTE, value
    end if
end macro

; ── The file ──────────────────────────────────────────────

macro axml_file? body?*
    local i

    ;  Phase one: intern, and count.  The namespace strings are
    ;  interned first so that the declaration can be written
    ;  before the tree.
    __ax_phase = $01
    __ax_elem = $00
    __ax_depth = $00
    _ax_ires "android"
    _ax_ires AXML_NS
    body

    _ax_ires "android"
    __ax_prefix = __ax_idx
    _ax_ires AXML_NS
    __ax_nsidx = __ax_idx

  axml_start:
    ushort RES_XML
    ushort $08
    uint   (axml_end - axml_start)

    ; ── string pool ───────────────────────────────────────
  pool_start:
    ushort RES_STRING_POOL
    ushort $1C
    uint   (pool_end - pool_start)
    uint   (__axr_n + __axo_n)            ; stringCount
    uint   $00                            ; styleCount
    uint   UTF8_FLAG
    uint   (pool_data - pool_start)
    uint   $00                            ; stylesStart
  pool_header_end:
    repeat __axr_n, i:0
        uint (pool_r_#i - pool_data)
    end repeat
    repeat __axo_n, i:0
        uint (pool_o_#i - pool_data)
    end repeat
  pool_data:
    ;  The two lengths are not the same number: the first counts
    ;  UTF-16 units, the second bytes, and they agree only for
    ;  ASCII.  _pool_string is forgeres.inc's, and is shared rather
    ;  than copied because this pool used to have its own copy of
    ;  the encoding — with the same mistake in it, fixed there and
    ;  not here.
    repeat __axr_n, i:0
      pool_r_#i:
        _pool_string __axr_v_#i
    end repeat
    repeat __axo_n, i:0
      pool_o_#i:
        _pool_string __axo_v_#i
    end repeat
    align $04
  pool_end:

    ; ── resource map ──────────────────────────────────────
    ;
    ;  One id per attribute name, in the order the names were
    ;  interned, which is the order they sit in the pool.
  map_start:
    ushort RES_XML_RESOURCE_MAP
    ushort $08
    uint   (map_end - map_start)
    repeat __axr_n, i:0
        uint __axr_i_#i
    end repeat
  map_end:

    ; ── the tree ──────────────────────────────────────────
    ushort RES_XML_START_NS
    ushort $10
    uint   $18
    uint   $01
    uint   $FFFFFFFF
    uint   __ax_prefix
    uint   __ax_nsidx

    __ax_phase = $02
    __ax_elem = $00
    __ax_depth = $00
    body

    ushort RES_XML_END_NS
    ushort $10
    uint   $18
    uint   $01
    uint   $FFFFFFFF
    uint   __ax_prefix
    uint   __ax_nsidx

  axml_end:
    __ax_phase = $00
end macro

; ── Framework attribute ids ───────────────────────────────
;
;  Read out of /system/framework/framework-res.apk on a device:
;  the android package is 0x01, attributes are type 0x01, and an
;  attribute's id is its position among them.  These are fixed
;  once published, so the table is a table and not a lookup.
;
;  Add what you need; anything missing is an assembly error rather
;  than an attribute that quietly does nothing.

defattr "theme",                        $01010000
defattr "label",                        $01010001
defattr "icon",                         $01010002
defattr "name",                         $01010003
defattr "permission",                   $01010006
defattr "enabled",                      $0101000E
defattr "debuggable",                   $0101000F
defattr "exported",                     $01010010
defattr "process",                      $01010011
defattr "authorities",                  $01010018
defattr "value",                        $01010024
defattr "resource",                     $01010025
defattr "launchMode",                   $0101001D
defattr "screenOrientation",            $0101001E
defattr "configChanges",                $0101001F
defattr "minSdkVersion",                $0101020C
defattr "versionCode",                  $0101021B
defattr "versionName",                  $0101021C
defattr "targetSdkVersion",             $01010270
defattr "maxSdkVersion",                $01010271
defattr "allowBackup",                  $01010280
defattr "required",                     $0101028E
defattr "installLocation",              $010102B7
defattr "hardwareAccelerated",          $010102D3
defattr "largeHeap",                    $0101035A
defattr "supportsRtl",                  $010103AF
defattr "banner",                       $010103F2
defattr "usesCleartextTraffic",         $010104EC
defattr "roundIcon",                    $0101052C
defattr "requestLegacyExternalStorage", $01010603

; ── and the ones a layout is written with ─────────────────

defattr "layout_width",                 $010100F4
defattr "layout_height",                $010100F5
defattr "layout_weight",                $01010181
defattr "layout_margin",                $010100F6
defattr "layout_marginTop",             $010100F8
defattr "layout_marginBottom",          $010100FA
defattr "layout_gravity",               $010100B3
defattr "orientation",                  $010100C4
defattr "gravity",                      $010100AF
defattr "text",                         $0101014F
defattr "textSize",                     $01010095
defattr "textColor",                    $01010098
defattr "textStyle",                    $01010097
defattr "hint",                         $01010150
defattr "background",                   $010100D4
defattr "padding",                      $010100D5
defattr "checked",                      $01010106
defattr "progress",                     $01010137
defattr "max",                          $01010136
defattr "rating",                       $01010145
defattr "numStars",                     $01010144
defattr "stepSize",                     $01010146
defattr "indeterminate",                $01010139
defattr "src",                          $01010119
defattr "id",                           $010100D0
defattr "singleLine",                   $0101015D

; The horizontal progress bar is reached through the theme, not
; through an attribute of its own: style="?android:attr/..." .
PROGRESS_BAR_HORIZONTAL? := $01010078

end if
