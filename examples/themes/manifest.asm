; ══════════════════════════════════════════════════════════
;  AndroidManifest.xml — thirteen activities, one per theme
;
;  android:theme is what actually chooses a theme, and it is
;  written here rather than in any code: a reference to a style in
;  the android package, resolved by the platform when the window
;  is created.  The list is walked with a macro of this file's
;  own, so the activities and the classes in themes.asm cannot
;  drift apart — they come out of the same thirteen lines.
;
;  T0 is the launcher and the only exported one; the rest are
;  reached from it by swiping, which needs no export at all.
; ══════════════════════════════════════════════════════════

format binary as "axml"
org $00

include '../../forgeaxml.inc'
include '../../tools/manifest.inc'
include 'ids.inc'
include 'list.inc'


;  The attributes of an element go in by ascending resource id,
;  which is the order aapt writes them in and the order the
;  platform reads them back in: it walks an element's attributes
;  with a cursor that only moves forwards, so one written out of
;  turn is stepped over and silently lost.  theme is $01010000 —
;  the very first attribute the framework ever published — so it
;  has to come before every other attribute here, and putting it
;  where it reads best is exactly how to lose it.
macro _activity? label?*, style?*
    theme_name __ti
    element "activity"
        attr_ref "theme", style                     ; $01010000
        attr_str "label", label                     ; $01010001
        attr_str "name", __nam                      ; $01010003
        if __ti = $00
            attr_bool "exported", 1
            element "intent-filter"
                element "action"
                    attr_str "name", "android.intent.action.MAIN"
                end element
                element "category"
                    attr_str "name", "android.intent.category.LAUNCHER"
                end element
            end element
        end if
    end element
    __ti = __ti + $01
end macro

macro _manifest
    element "manifest"
        attr_plain "package", "app.hello.themes"

        sdk_versions 21, 33

        element "application"
            attr_str "label", "Themes"
            attr_ref "icon", R_DRAWABLE_ICON
            __ti = $00
            each_theme _activity
        end element
    end element
end macro

axml_file _manifest
