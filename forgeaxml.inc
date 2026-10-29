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
    _intern __axt, add, name,, id
end macro

; ── The two string pools ──────────────────────────────────
;
;  Attribute names go in the first, everything else in the second,
;  and the pool written out is one after the other.  Keeping them
;  apart is what lets the resource map stay parallel to the front
;  of the pool.

__axr_n = $00               ; attribute names
__axo_n = $00               ; all other strings

; A string's place in the written pool is its rank, which is only
; known once the tree has been walked: the attribute names by
; resource id, then everything else in the order dex strings take.
; So neither the order attributes are written in nor the order
; elements come in moves a string.  These two give a string its
; final index while the tree is still being walked: the ranks, and
; the count of attribute names the second pool sits behind, are set
; after the walk, and fasmg takes a symbol defined further on from
; the pass before.

macro _ax_ires? value?*
    _intern __axo, add, value, lengthof value
    repeat 1, e:__axo_found
        __ax_idx = __axr_total + __axo_r_#e
    end repeat
end macro

macro _ax_ires_attr? name?*
    _intern __axt, find, name
    if __axt_found = -1
        err 'unknown android attribute: ', name, ' — add it to the table in forgeaxml.inc'
    else
        repeat 1, k:__axt_found
            __ax_id = __axt_i_#k
        end repeat
    end if
    _intern __axr, add, name,, __ax_id
    repeat 1, e:__axr_found
        __ax_idx = __axr_r_#e
    end repeat
end macro

; ── The description, walked once ──────────────────────────
;
;  The tree is written as it is walked, into an area of its own,
;  with every string already at its final index; axml_file copies it
;  out behind the string pool, which can only be written once the
;  walk has met every string.

__ax_elem  = $00            ; element ordinal, in visiting order
__ax_depth = $00
__axa_open = $00            ; an element's header is waiting for its attributes

; An index has to become a repeat counter before # can paste it:
; the operator joins tokens, not the values behind them.
macro _ax_open
    repeat 1, d:__ax_depth
        __ax_stack_#d = __ax_elem
    end repeat
end macro

macro element? name?*
    _ax_flush
    _intern __axo, add, name, lengthof name
    _ax_open
    repeat 1, e:__ax_elem
        __ax_name_#e = __axo_found
    end repeat
    repeat 1, k:__axo_found
        __axa_name = __axr_total + __axo_r_#k
    end repeat
    __ax_elem = __ax_elem + $01
    __ax_depth = __ax_depth + $01
    __axa_open = $01
    __axa_n = $00
    __axa_style_at = -$01
end macro

;  An element's header and its attributes are written together,
;  once the attributes are all in: when the next element starts, or
;  this one ends.  By then they can be put in order, and the header
;  can say where style= went.
;
;  The order is aapt's.  Those with a resource id come first, by
;  ascending id, and that is not a matter of taste: the platform
;  resolves an element's attributes with a cursor that only walks
;  forwards, so one out of turn is stepped over — read, and then not
;  used.  Those without an id — package=, style= — follow, in the
;  order they were written.  The platform finds style= through the
;  index in the header and nothing else.
macro _ax_flush
    if __axa_open
        __axa_open = $00
        _order __axa_k_, _key_less, __axa_r_, __axa_o_, __axa_n, \
            'an attribute is written twice in one element'
        __axa_style = $00
        if __axa_style_at >= $00
            repeat 1, i:__axa_style_at
                __axa_style = __axa_r_#i + $01
            end repeat
        end if

        ushort RES_XML_START_ELEMENT
        ushort $10                        ; header size
        uint   $10 + $14 + $14 * __axa_n
        uint   $01                        ; line number
        uint   $FFFFFFFF                  ; comment: none
        uint   $FFFFFFFF                  ; namespace of the element
        uint   __axa_name
        ushort $14                        ; where attributes start
        ushort $14                        ; size of one attribute
        ushort __axa_n
        ushort $00                        ; id attribute index
        ushort $00                        ; class attribute index
        ushort __axa_style                ; style attribute index, from 1
        repeat __axa_n, r:$00
            repeat 1, i:__axa_o_#r
                _ax_emit __axa_ns_#i, __axa_nm_#i, __axa_raw_#i, __axa_ty_#i, __axa_d_#i
            end repeat
        end repeat
    end if
end macro

;  One attribute, kept until _ax_flush writes them all.  key is the
;  resource id, or for one without an id a number above every id,
;  so that it sorts after them and in the order it was written.
macro _ax_attr? key?*, nsidx?*, nameidx?*, raw?*, type?*, data?*
    ;  Once an element inside this one has started, this one's header
    ;  and attributes are written, and there is no adding to them.
    if __axa_open = $00
        err 'an attribute has to come before the elements inside the one it belongs to'
    end if
    repeat 1, i:__axa_n
        __axa_k_#i   = key
        __axa_ns_#i  = nsidx
        __axa_nm_#i  = nameidx
        __axa_raw_#i = raw
        __axa_ty_#i  = type
        __axa_d_#i   = data
    end repeat
    __axa_n = __axa_n + $01
end macro

macro end?.element
    _ax_flush
    __ax_depth = __ax_depth - $01
    repeat 1, d:__ax_depth
        __ax_elem_tmp = __ax_stack_#d
    end repeat
    repeat 1, e:__ax_elem_tmp
        repeat 1, k:__ax_name_#e
            __ax_name_tmp = __axr_total + __axo_r_#k
        end repeat
    end repeat

    ushort RES_XML_END_ELEMENT
    ushort $10
    uint   $18
    uint   $01
    uint   $FFFFFFFF
    uint   $FFFFFFFF
    uint   __ax_name_tmp
end macro

; ── Attributes ────────────────────────────────────────────

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
    _ax_ires_attr name
    __ax_n_tmp = __ax_idx
    _ax_ires value
    __ax_v_tmp = __ax_idx
    _ax_ires AXML_NS
    _ax_attr __ax_id, __ax_idx, __ax_n_tmp, __ax_v_tmp, TYPE_STRING, __ax_v_tmp
end macro

;  package= on <manifest> is the one attribute with no namespace
;  and no resource id.
macro attr_plain? name?*, value?*
    _ax_ires name
    __ax_n_tmp = __ax_idx
    _ax_ires value
    _ax_attr ($01 shl $20) + __axa_n, $FFFFFFFF, __ax_n_tmp, __ax_idx, TYPE_STRING, __ax_idx
end macro

macro _ax_typed? name?*, type?*, data?*
    _ax_ires_attr name
    __ax_n_tmp = __ax_idx
    _ax_ires AXML_NS
    _ax_attr __ax_id, __ax_idx, __ax_n_tmp, $FFFFFFFF, type, data
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
    _ax_ires "style"
    __axa_style_at = __axa_n
    _ax_attr ($01 shl $20) + __axa_n, $FFFFFFFF, __ax_idx, $FFFFFFFF, TYPE_ATTRIBUTE, value
end macro

; ── The file ──────────────────────────────────────────────

macro axml_file? body?*
    local i

    ;  The tree, walked once into an area of its own, with the
    ;  namespace strings interned first.
    __ax_elem = $00
    __ax_depth = $00
    __axa_open = $00
    _ax_ires "android"
    _ax_ires AXML_NS
    virtual at $00
    __ax_tree::
        body
        _ax_flush
        __ax_tree_size = $
    end virtual

    _order __axr_i_, _key_less, __axr_r_, __axr_o_, __axr_n, \
        'two attribute names have one resource id'
    _order __axo, _str_less, __axo_r_, __axo_o_, __axo_n, \
        'string pool came out unsorted'
    __axr_total = __axr_n

    _ax_ires "android"
    __ax_prefix = __ax_idx
    _ax_ires AXML_NS
    __ax_nsidx = __ax_idx

    ;  Every block here is a chunk as forgeres.inc writes one: the
    ;  header size and the total size are distances between labels,
    ;  so neither is a number to keep in step with the fields.
    chunk axml, RES_XML
  axml_header_end:

    ; ── string pool ───────────────────────────────────────
    chunk pool, RES_STRING_POOL
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
        repeat 1, k:__axr_o_#i
            _pool_string __axr_v_#k
        end repeat
    end repeat
    repeat __axo_n, i:0
      pool_o_#i:
        repeat 1, k:__axo_o_#i
            _pool_string __axo_v_#k
        end repeat
    end repeat
    align $04
  pool_end:

    ; ── resource map ──────────────────────────────────────
    ;
    ;  One id per attribute name, in the order they sit in the
    ;  pool, which is ascending id.
    chunk map, RES_XML_RESOURCE_MAP
  map_header_end:
    repeat __axr_n, i:0
        repeat 1, k:__axr_o_#i
            uint __axr_i_#k
        end repeat
    end repeat
  map_end:

    ; ── the tree ──────────────────────────────────────────
    chunk startns, RES_XML_START_NS
    uint   $01                            ; line number
    uint   $FFFFFFFF                      ; comment: none
  startns_header_end:
    uint   __ax_prefix
    uint   __ax_nsidx
  startns_end:

    ;  The tree, as it was written while it was walked.
    if __ax_tree_size
        load __ax_tree_bytes : __ax_tree_size from __ax_tree : $00
        emit __ax_tree_size : __ax_tree_bytes
    end if

    chunk endns, RES_XML_END_NS
    uint   $01
    uint   $FFFFFFFF
  endns_header_end:
    uint   __ax_prefix
    uint   __ax_nsidx
  endns_end:

  axml_end:
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
