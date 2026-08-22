# IOCCC 2020 — "Most phony"

`prog.c` and `dtmf.bin` in this directory are the winning entry
**Most phony** from the 27th International Obfuscated C Code Contest
(2020), by **Edward Giles**.

* Entry: <https://www.ioccc.org/2020/giles/index.html>
* Sources: <https://github.com/ioccc-src/winner/tree/master/2020/giles>

They are here **unmodified**. Verify with:

```sh
sha256sum prog.c dtmf.bin
507a14aaa40f20386fe76946c0721763093a56cda5ed7edc53c19184ac1c3544  prog.c
c1112a0c0babf2551dad1d2e98801047b7f469611203dc03aee54409a3e6b55f  dtmf.bin
```

The program encodes and decodes DTMF — telephone touch tones. Its
source, ignoring the preprocessor lines, is laid out in the shape of a
telephone handset, and its size under the IOCCC size tool is 1963, the
year push-button telephones went on sale.

## How this example uses it

It is compiled exactly as submitted. Two build flags do all the
adapting, and neither touches the file:

* `-I shim` puts a `<stdio.h>` and a `<string.h>` of our own in front
  of the system headers. They implement the fifteen functions the
  entry calls, over plain memory instead of over a filesystem — so it
  runs inside an APK, with no libc, and never learns the difference.
* `-Dmain=dtmf_main` renames its `main` so the JNI side can call it
  the way a shell would.

`dtmf.bin` is the entry's own data file. It travels in the APK under
`assets/`, and the Dalvik side reads it out and hands it down.

## Licence

The entry is licensed by Landon Curt Noll under
**CC BY-SA 4.0** (Creative Commons Attribution-ShareAlike 4.0
International), copyright 2020–2024.

Full text: <https://creativecommons.org/licenses/by-sa/4.0/legalcode>

This is the one directory in forgedex not under the repository's MIT
licence. Share-alike applies to adaptations of the entry; the files
here are unmodified, and the rest of the repository is a separate
work.
