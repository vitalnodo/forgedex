; ══════════════════════════════════════════════════════════
;  resources.arsc for the gallery.
;
;  What is in it is in ids.inc; this only says where the package
;  begins.  forge/resources.inc walks that declaration once and
;  lays out the pools, the type specs and every entry from it.
; ══════════════════════════════════════════════════════════

format binary as "arsc"
org $00

include '../../forgeres.inc'
include 'ids.inc'

resource_table "app.hello.themes"
