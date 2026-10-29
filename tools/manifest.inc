; ══════════════════════════════════════════════════════════
;  The manifest every example shares.
;
;  Nine of them differ in three values and nothing else: the
;  package, the label, and the name of the one activity.  So they
;  say only that:
;
;      format binary as "axml"
;      org $00
;
;      include '../../forgeaxml.inc'
;      include '../../tools/manifest.inc'
;
;      app_manifest "app.hello.canvas", "Canvas", "CanvasDemo"
;
;  An example that needs more than one activity, or a permission,
;  writes its own manifest.asm instead — fetch and themes both do.
;  Bending this macro to cover them would cost more than the thirty
;  lines it would save.
;
;  forgeaxml.inc is the only include needed: it pulls in the
;  resource-chunk constants and, through them, the primitives.
; ══════════════════════════════════════════════════════════

; ── The order of attributes ───────────────────────────────
;
;  The platform resolves an element's attributes with a cursor that
;  only walks forwards, so one written out of turn is read and then
;  stepped over.  forgeaxml.inc sorts them the way aapt does, by
;  resource id, so they are written here in whatever order reads
;  best.

;  The one block every manifest has, wherever the rest of it comes
;  from.  The element and the two attribute names live here so that
;  an example writing its own manifest still gets them from one
;  place; the numbers are its own business.
;
;  Both defaults are meant to be argued with.  21 is Android 5.0 and
;  33 is Android 13; an example takes its own pair when it wants one.
macro sdk_versions? minsdk:21, targetsdk:33
    element "uses-sdk"
        attr_int "minSdkVersion", minsdk
        attr_int "targetSdkVersion", targetsdk
    end element
end macro

macro app_manifest? package?*, label?*, activity?*, minsdk:21, targetsdk:33
    macro _generated_manifest
        element "manifest"
            attr_plain "package", package

            sdk_versions minsdk, targetsdk

            element "application"
                attr_str "label", label

                ;  Exported, because the launcher is another app and
                ;  from API 31 the flag has to be written down rather
                ;  than inferred from having an intent-filter.
                element "activity"
                    attr_str "name", activity
                    attr_bool "exported", 1
                    element "intent-filter"
                        element "action"
                            attr_str "name", "android.intent.action.MAIN"
                        end element
                        element "category"
                            attr_str "name", "android.intent.category.LAUNCHER"
                        end element
                    end element
                end element
            end element
        end element
    end macro

    axml_file _generated_manifest
end macro
