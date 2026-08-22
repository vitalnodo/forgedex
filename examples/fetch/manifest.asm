; ══════════════════════════════════════════════════════════
;  AndroidManifest.xml — two activities and a permission
;
;  The only example that does not fit app_manifest: it asks for the
;  network, and it has a second screen that the launcher must not
;  offer.  Page is reached from Main by an Intent, which needs no
;  intent-filter and no export.
;
;  The uses-sdk block still comes from tools/manifest.inc — writing
;  a manifest by hand is no reason to spell that one twice.
; ══════════════════════════════════════════════════════════

format binary as "axml"
org $00

include '../../forgeaxml.inc'
include '../../tools/manifest.inc'

macro _manifest
    element "manifest"
        attr_plain "package", "app.hello.fetch"

        sdk_versions 21, 33

        element "uses-permission"
            attr_str "name", "android.permission.INTERNET"
        end element

        element "application"
            attr_str "label", "Fetch"

            element "activity"
                attr_str "name", "Main"
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

            element "activity"
                attr_str "name", "Page"
            end element
        end element
    end element
end macro

axml_file _manifest
