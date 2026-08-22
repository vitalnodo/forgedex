; ══════════════════════════════════════════════════════════
;  The gallery, named once
;
;  A theme belongs to an Activity, not to a View: it is chosen
;  when the window is created and cannot be changed afterwards.
;  So a gallery of thirteen themes is a gallery of thirteen
;  activities, and this is the list all of them come from.
;
;  themes.asm hands it a macro that writes a class, manifest.asm
;  hands it one that writes an <activity>, and neither of them
;  holds the list itself.  An entry is a name to show and the
;  framework's id for the style; the class is called T0, T1, … and
;  the number is where the entry sits, so nothing here has to say
;  a class name twice.
;
;  The ids are real resource ids out of the android package —
;  package $01, type $03 (style) — read from
;  platforms/android-*/data/res/values/public.xml, which is what
;  aapt itself resolves @android:style/… against.
; ══════════════════════════════════════════════════════════

THEME_COUNT? := 13

macro each_theme? handler?*
    handler "Theme.Material",                       $01030224
    handler "Theme.Material.Light",                 $01030237
    handler "Theme.Material.Light.DarkActionBar",   $01030238
    handler "Theme.Material.NoActionBar.Fullscreen",$0103022F
    handler "Theme.Material.Dialog",                $01030225
    handler "Theme.Material.Light.Dialog.Alert",    $0103023A
    handler "Theme.Holo",                           $0103006B
    handler "Theme.Holo.Light.DarkActionBar",       $01030119
    handler "Theme.DeviceDefault",                  $01030128
    handler "Theme.DeviceDefault.Light",            $0103012B
    handler "Theme.Black.NoTitleBar",               $01030009
    handler "Theme.Light",                          $0103000C
    handler "Theme.Translucent",                    $0103000F
end macro

;  T0 … T12, built from the index rather than written out.  An
;  index has to become a repeat counter before ` can turn it into
;  text — the same reason forgedex.inc reaches its interning
;  tables through a repeat.
macro theme_class? i?*
    repeat 1, n:(i)
        virtual at $00
            db "Lapp/hello/themes/T", `n, ";"
            load __cls : $ - $$ from $00
        end virtual
    end repeat
end macro

;  The same name as the manifest writes it: no dots at all, so the
;  platform prepends the package to reach app.hello.themes.T3.
macro theme_name? i?*
    repeat 1, n:(i)
        virtual at $00
            db "T", `n
            load __nam : $ - $$ from $00
        end virtual
    end repeat
end macro
