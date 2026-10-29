; ══════════════════════════════════════════════════════════
;  forgeres.inc — a resources.arsc, hand-forged
;
;  An APK cannot have an icon without one of these.  The manifest
;  names the icon as a reference — a number like $7F010000, not a
;  path — and resources.arsc is the table that says which file
;  that number means.  aapt is what normally writes it.
;
;  The format is chunked the same way binary AXML is: every piece
;  starts with a type, the size of its own header, and the size of
;  the whole chunk including anything nested inside it.  What it
;  does not have is any of what makes dex hard — no LEB128, no
;  pools that index each other, no checksum over the result.
;
;  Layout of the smallest table that works:
;
;      RES_TABLE
;      ├── string pool          the values: file paths
;      └── PACKAGE  id $7F
;          ├── string pool      type names:  "drawable"
;          ├── string pool      key names:   "icon"
;          ├── TYPE_SPEC        one entry, one configuration
;          └── TYPE             the entry itself, pointing at a path
;
;  A resource id is built out of where the thing sits:
;
;      $7F      01       0000
;      package  type     entry
;
;  so the first entry of the first type of our own package is
;  $7F010000, and that is the number the manifest carries.
; ══════════════════════════════════════════════════════════

if ~ definite __forgeres_included
__forgeres_included := $01

include 'forge/base.inc'

; ── Chunk headers ─────────────────────────────────────────
;
;  A chunk knows two sizes: its header and its whole self.  Both
;  are filled in afterwards, once the end is known, which is what
;  the labels are for.

RES_STRING_POOL?  := $0001
RES_TABLE?        := $0002
RES_TABLE_PACKAGE? := $0200
RES_TABLE_TYPE?   := $0201
RES_TABLE_TYPE_SPEC? := $0202

; ── Res_value data types ──────────────────────────────────
;
;  What the four bytes of a value mean.  Shared with binary AXML,
;  which is the same chunk format and reads the same table — an
;  attribute in a layout and an entry in this table carry the very
;  same eight bytes.

TYPE_REFERENCE?       := $01    ; another resource, by id
TYPE_ATTRIBUTE?       := $02
TYPE_STRING?          := $03    ; an index into a string pool
TYPE_FLOAT?           := $04
TYPE_DIMENSION?       := $05    ; a number and a unit
TYPE_INT_DEC?         := $10
TYPE_INT_BOOLEAN?     := $12    ; $FFFFFFFF is true, $00 is false
TYPE_INT_COLOR_ARGB8? := $1C

; String pool flags
UTF8_FLAG?        := $0100

macro chunk? name?*, type?*
    name#_start:
        ushort type
        ushort (name#_header_end - name#_start)
        uint   (name#_end - name#_start)
end macro

; ── String pools ──────────────────────────────────────────
;
;  Written UTF-8.  Each string carries two lengths before it — how
;  many UTF-16 units it would take, then how many bytes it does
;  take — and a NUL after it, which nothing counts but everything
;  expects.
;
;  The two lengths are the same number only for ASCII, which is why
;  writing one for both went unnoticed for as long as every string
;  here was a file path.  "Привіт" is six characters and twelve
;  bytes, and a pool claiming twelve UTF-16 units is wrong in the
;  silent way: the table assembles, the APK installs, and the text
;  comes back mangled.
;
;  A length of 128 or more does not fit a byte either.  The format
;  writes those as two, the first with its high bit set:
;
;      len < $80        one byte
;      otherwise        $80 or (len shr 8), then len and $FF

macro _res_length? n?*
    if n < $80
        ubyte n
    else
        ubyte ($80 or (n shr $08))
        ubyte (n and $FF)
    end if
end macro

;  One string as a pool holds it: how many UTF-16 units it would
;  take, how many bytes it does take, the bytes, and a NUL that
;  nothing counts and everything expects.
;
;  Shared with forgeaxml.inc, which writes a pool of its own and
;  used to carry a second copy of this.
macro _pool_string? s?*
    _utf16_length __sp_u16, s, lengthof s
    _res_length __sp_u16
    _res_length lengthof s
    db    s
    ubyte $00
end macro

macro string_pool? name?*, strings?*&
    local count

    count = 0
    iterate s, strings
        count = count + 1
    end iterate

    chunk name, RES_STRING_POOL
        uint  count                      ; stringCount
        uint  $00                        ; styleCount
        uint  UTF8_FLAG                  ; flags
        uint  (name#_data - name#_start) ; stringsStart
        uint  $00                        ; stylesStart
    name#_header_end:

        ;  The offsets are relative to where the data begins, so
        ;  they can be worked out only once the strings are laid
        ;  out.  fasmg resolves the forward references itself.
        ;  Inside an iterate the index is %, and # pastes it into a
        ;  label name — the offsets are written before the strings
        ;  they point at, so both loops have to agree on those names.
        iterate s, strings
            uint (name#_str_#% - name#_data)
        end iterate

    name#_data:
        iterate s, strings
            name#_str_#%:
                _pool_string s
        end iterate

        align $04
    name#_end:
end macro

; ── Configurations ────────────────────────────────────────
;
;  A ResTable_config says which locale, density, orientation and
;  so on a set of entries is for, and it is the whole reason this
;  file format exists.  One id can have several values, each in a
;  TYPE chunk of its own with a different config; the platform
;  compares the device against each and picks the best match when
;  the resource is asked for, not when the APK is built.
;
;  All zero means "any" — it matches everything, and loses to any
;  config that also matches and says more.
;
;  The struct is 56 bytes and mostly unused here.  What the fields
;  are, by offset, so the two that are written can be found again:
;
;      0   size            16  input
;      4   imsi            20  screenSize
;      8   locale          24  sdkVersion, minorVersion
;      12  screenType      28  screenLayout, uiMode, smallestWidthDp
;                          32  and on to 56
;
;  uiMode carries the night bits in $30: $10 is notnight, $20 is
;  night, $00 is either.  It is the qualifier `-night` compiles to.
;
;  The language is two ASCII letters at offset 8, and the country
;  two more after it.  `-uk` is nothing more than 'u','k' sitting
;  there; the packed form with the high bit is only for the
;  three-letter codes, which nothing here uses.

RES_CONFIG_SIZE? := 56

UI_MODE_NIGHT_ANY? := $00
UI_MODE_NIGHT_NO?  := $10
UI_MODE_NIGHT_YES? := $20

;  A language of "" means any, which is what an unqualified set of
;  values is: it matches every device and loses to one that names
;  the language the device is actually set to.
macro config? lang:"", ui_mode:$00, sdk:$00
    uint   RES_CONFIG_SIZE
    uint   $00                  ; imsi
    ;  `db ""` emits nothing at all, so the any case has to be
    ;  written as the two zero bytes it is rather than left to the
    ;  string — otherwise every field after this one shifts.
    match ="", lang
        ushort $00              ; language: any
    else
        db lang                 ; language: two letters
    end match
    ushort $00                  ; country: any
    uint   $00                  ; screenType
    uint   $00                  ; input
    uint   $00                  ; screenSize
    ushort sdk                  ; sdkVersion
    ushort $00                  ; minorVersion
    ubyte  $00                  ; screenLayout
    ubyte  ui_mode              ; uiMode
    ushort $00                  ; smallestScreenWidthDp
    repeat RES_CONFIG_SIZE - $20
        ubyte $00
    end repeat
end macro

macro default_config
    config
end macro

;  The table above is the format; forge/resources.inc is how a
;  declaration turns into one.
include 'forge/resources.inc'

end if
