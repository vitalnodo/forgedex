; ══════════════════════════════════════════════════════════
;  res/layout/screen.xml — the one layout, under every theme
;
;  The widgets example wrote its own colours and sizes into the
;  attributes, which is why it looks the same wherever it runs.
;  This one writes none: no background, no textColor, nothing that
;  would answer a question the theme is there to answer.  What is
;  left is structure, and thirteen windows put thirteen different
;  answers behind it.
;
;  The one id in the file is @id/label — $7F030000, minted by
;  res.asm — so that each activity can find that TextView and put
;  the name of its own theme in it.  The rest are inflated,
;  looked at, and never spoken to again.
; ══════════════════════════════════════════════════════════

format binary as "axml"
org $00

include '../../forgeaxml.inc'
include 'ids.inc'


macro _row_open
    element "LinearLayout"
        attr_int   "orientation", 0                  ; horizontal
        attr_dp    "padding", 8
        attr_int   "layout_width", MATCH_PARENT
        attr_int   "layout_height", WRAP_CONTENT
end macro

macro _label text
    element "TextView"
        attr_sp    "textSize", 13
        attr_dp    "padding", 8
        attr_int   "layout_width", WRAP_CONTENT
        attr_int   "layout_height", WRAP_CONTENT
        attr_str   "text", text
    end element
end macro

macro _layout
    element "ScrollView"
        attr_int "layout_width", MATCH_PARENT
        attr_int "layout_height", MATCH_PARENT

        element "LinearLayout"
            attr_int "orientation", 1                ; vertical
            attr_dp  "padding", 12
            attr_int "layout_width", MATCH_PARENT
            attr_int "layout_height", WRAP_CONTENT

            ;  The only view anybody asks for by name.  id is
            ;  $010100D0, so it sits between textSize and padding
            ;  rather than at the top where it would read better:
            ;  attributes are written by ascending resource id or
            ;  they are not read at all.
            element "TextView"
                attr_sp  "textSize", 22             ; $01010095
                attr_ref "id", R_ID_LABEL           ; $010100D0
                attr_dp  "padding", 12              ; $010100D5
                attr_int "layout_width", MATCH_PARENT
                attr_int "layout_height", WRAP_CONTENT
                attr_str "text", "a theme"
            end element

            _label "◀  swipe  ▶"

            _label "text entry"
            element "EditText"
                attr_int "layout_width", MATCH_PARENT
                attr_int "layout_height", WRAP_CONTENT
                attr_str "hint", "type here"
                attr_int "singleLine", 1
            end element

            _label "buttons"
            _row_open
                element "Button"
                    attr_int   "layout_width", 0
                    attr_int   "layout_height", WRAP_CONTENT
                    attr_str   "text", "one"
                    attr_float "layout_weight", 1.0
                end element
                element "Button"
                    attr_int   "layout_width", 0
                    attr_int   "layout_height", WRAP_CONTENT
                    attr_str   "text", "two"
                    attr_float "layout_weight", 1.0
                end element
                element "Button"
                    attr_int   "layout_width", 0
                    attr_int   "layout_height", WRAP_CONTENT
                    attr_str   "text", "three"
                    attr_float "layout_weight", 1.0
                end element
            end element

            _label "switches"
            _row_open
                element "CheckBox"
                    attr_int  "layout_width", WRAP_CONTENT
                    attr_int  "layout_height", WRAP_CONTENT
                    attr_bool "checked", 1
                    attr_str  "text", "checkbox"
                end element
                element "Switch"
                    attr_int "layout_width", WRAP_CONTENT
                    attr_int "layout_height", WRAP_CONTENT
                    attr_dp  "layout_margin", 16
                    attr_str "text", "switch"
                end element
            end element

            _label "one of many"
            element "RadioGroup"
                attr_int "orientation", 0
                attr_dp  "padding", 8
                attr_int "layout_width", MATCH_PARENT
                attr_int "layout_height", WRAP_CONTENT
                element "RadioButton"
                    attr_int  "layout_width", WRAP_CONTENT
                    attr_int  "layout_height", WRAP_CONTENT
                    attr_bool "checked", 1
                    attr_str  "text", "this"
                end element
                element "RadioButton"
                    attr_int "layout_width", WRAP_CONTENT
                    attr_int "layout_height", WRAP_CONTENT
                    attr_dp  "layout_margin", 16
                    attr_str "text", "that"
                end element
            end element

            _label "a slider"
            element "SeekBar"
                attr_dp  "padding", 8
                attr_int "layout_width", MATCH_PARENT
                attr_int "layout_height", WRAP_CONTENT
                attr_int "max", 100
                attr_int "progress", 62
            end element

            _label "progress, both kinds"
            element "ProgressBar"
                attr_style PROGRESS_BAR_HORIZONTAL
                attr_int "layout_width", MATCH_PARENT
                attr_int "layout_height", WRAP_CONTENT
                attr_int "max", 100
                attr_int "progress", 40
            end element
            element "ProgressBar"
                attr_int "layout_width", WRAP_CONTENT
                attr_int "layout_height", WRAP_CONTENT
                attr_dp  "layout_margin", 12
            end element

            _label "stars"
            element "RatingBar"
                attr_int   "layout_width", WRAP_CONTENT
                attr_int   "layout_height", WRAP_CONTENT
                attr_int   "numStars", 5
                attr_float "rating", 3.5
                attr_float "stepSize", 0.5
            end element

            _label "a clock and a stopwatch"
            _row_open
                element "AnalogClock"
                    attr_int "layout_width", WRAP_CONTENT
                    attr_int "layout_height", WRAP_CONTENT
                end element
                element "Chronometer"
                    attr_sp  "textSize", 28
                    attr_int "layout_width", WRAP_CONTENT
                    attr_int "layout_height", WRAP_CONTENT
                    attr_dp  "layout_margin", 16
                end element
            end element

            _label "and the icon this was built with"
            element "ImageView"
                attr_dp  "padding", 12
                attr_int "layout_width", WRAP_CONTENT
                attr_int "layout_height", WRAP_CONTENT
                attr_ref "src", R_DRAWABLE_ICON
            end element
        end element
    end element
end macro

axml_file _layout
