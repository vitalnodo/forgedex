; ══════════════════════════════════════════════════════════
;  Every resource this example has, declared once
;
;  The order fixes the numbers: a type is its position in the type
;  pool counting from one, an entry its position within the type.
;  forge/resources.inc turns what is below into constants, three
;  string pools and the table itself, so no other file here holds a
;  number of its own.
;
;  Adding a button is a line in _id_entries; giving the icon a
;  night variant is a line in _drawable_configs.  Nothing else moves.
; ══════════════════════════════════════════════════════════

;  This declaration uses resource_ids, VALUE_* and cfg_*, so it
;  says where they come from rather than leaving it to whoever
;  includes it — a dex source needs them as much as res.asm does.
include '../../forgeres.inc'

macro each_type? handler?*
    ;        type name,   what a value is,  entries,           configurations
    handler "drawable",   VALUE_FILE,       _drawable_entries, _drawable_configs
    handler "layout",     VALUE_FILE,       _layout_entries,   _layout_configs
    handler "id",         VALUE_NONE,       _id_entries,       _id_configs
end macro

; ── What each type holds ──────────────────────────────────

macro _drawable_entries? handler?*
    handler R_DRAWABLE_ICON, "icon"
end macro

macro _layout_entries? handler?*
    handler R_LAYOUT_SCREEN, "screen"
end macro

;  An id resource names no file and holds no value: it exists so
;  that a number exists, which the layout writes into android:id
;  and findViewById then asks for.
macro _id_entries? handler?*
    handler R_ID_LABEL, "label"
end macro

; ── And in which configurations ───────────────────────────
;
;  One line per TYPE chunk: which configuration, then a value for
;  every entry above, in that order.

macro _drawable_configs? handler?*
    handler cfg_any, "res/drawable/icon.png"
end macro

macro _layout_configs? handler?*
    handler cfg_any, "res/layout/screen.xml"
end macro

macro _id_configs? handler?*
    handler cfg_any
end macro

resource_ids
