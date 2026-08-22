/*
 * Just enough <stdio.h> for the IOCCC entry, over memory.
 *
 * prog.c is compiled against this header instead of the system one,
 * so that it needs no libc at all: fopen hands back the bytes of
 * dtmf.bin that Java read out of the APK, and everything written to
 * stdout lands in a buffer we read afterwards.  Nothing here makes a
 * system call, which is what keeps libdtmf.so free of DT_NEEDED.
 *
 * These are declared as functions, never as macros, on purpose:
 * prog.c uses `memcpy` and `fread` as goto labels as well as calling
 * them, and a macro would break at the label.
 */

#ifndef DTMF_SHIM_STDIO_H
#define DTMF_SHIM_STDIO_H

#include <stddef.h>

typedef struct FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

FILE  *fopen(const char *name, const char *mode);
int    fclose(FILE *f);
size_t fread(void *p, size_t size, size_t count, FILE *f);
size_t fwrite(const void *p, size_t size, size_t count, FILE *f);
int    getc(FILE *f);
int    putchar(int c);
int    puts(const char *s);
int    ferror(FILE *f);
int    setvbuf(FILE *f, char *buf, int mode, size_t size);
int    fprintf(FILE *f, const char *fmt, ...);

/* ── The seams this shim adds ──────────────────────────────
 *
 *  The entry expects a file named dtmf.bin in the working
 *  directory and a WAV on stdin or stdout.  An APK has neither a
 *  working directory worth the name nor standard streams, so the
 *  caller supplies both by hand.
 */

void  dtmf_set_file(const unsigned char *data, long len);
void  dtmf_set_input(const unsigned char *data, long len);
void  dtmf_rewind_output(void);
long  dtmf_output(unsigned char **out);

int   dtmf_main(int argc, char **argv);   /* prog.c's main, renamed */

#endif
