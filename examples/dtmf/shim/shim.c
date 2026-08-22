/*
 * The bodies behind shim/stdio.h and shim/string.h.
 *
 * Three streams, all of them memory:
 *
 *   the file    what fopen returns, whatever name it is asked for.
 *               It holds the 253 bytes of dtmf.bin: a 44-byte WAV
 *               header to copy, 128 bytes of oscillator constants,
 *               and the 81-byte key table.
 *   the input   what stdin reads.  Encoding never touches it;
 *               decoding a WAV would.
 *   the output  what stdout and putchar collect.  Encoding leaves
 *               a whole WAV here.
 *
 * Compiled with -fno-builtin, so that our memcpy is not turned into
 * a call to itself.
 */

#include "stdio.h"
#include "string.h"

/* Room for a WAV of thirty-odd digits: the entry writes 3200
 * samples per digit at 16 kHz, which is 6400 bytes each. */
#define OUT_MAX (208L * 1024L)

struct FILE {
    const unsigned char *rd;   /* what fread and getc take from */
    unsigned char       *wr;   /* what fwrite and putchar add to */
    long                 pos;
    long                 len;  /* bytes readable, or bytes written */
    long                 cap;
};

static unsigned char out_buf[OUT_MAX];

static FILE file_f = { 0, 0, 0, 0, 0 };
static FILE file_i = { 0, 0, 0, 0, 0 };
static FILE file_o = { 0, out_buf, 0, 0, OUT_MAX };

FILE *stdin  = &file_i;
FILE *stdout = &file_o;
FILE *stderr = &file_o;

/* ── The seams ────────────────────────────────────────────── */

void dtmf_set_file(const unsigned char *data, long len)
{
    file_f.rd = data;
    file_f.pos = 0;
    file_f.len = len;
}

void dtmf_set_input(const unsigned char *data, long len)
{
    file_i.rd = data;
    file_i.pos = 0;
    file_i.len = len;
}

void dtmf_rewind_output(void)
{
    file_o.pos = 0;
    file_o.len = 0;
}

long dtmf_output(unsigned char **out)
{
    *out = out_buf;
    return file_o.len;
}

/* ── stdio ────────────────────────────────────────────────── */

/*
 * There is only one file to open, so the name is not looked at.
 * The entry asks for dtmf.bin and nothing else.
 */
FILE *fopen(const char *name, const char *mode)
{
    (void)name;
    (void)mode;
    if (!file_f.rd)
        return 0;
    file_f.pos = 0;
    return &file_f;
}

int fclose(FILE *f)
{
    (void)f;
    return 0;
}

/*
 * The entry always asks for one item and tests the result as a
 * flag, so a short read has to come back as 0 rather than as a
 * count of bytes.
 */
size_t fread(void *p, size_t size, size_t count, FILE *f)
{
    unsigned char *d = (unsigned char *)p;
    size_t want = size * count;
    size_t i;

    if (!f || !f->rd || f->pos + (long)want > f->len)
        return 0;

    for (i = 0; i < want; i++)
        d[i] = f->rd[f->pos + (long)i];
    f->pos += (long)want;
    return count;
}

size_t fwrite(const void *p, size_t size, size_t count, FILE *f)
{
    const unsigned char *s = (const unsigned char *)p;
    size_t want = size * count;
    size_t i;

    if (!f || !f->wr || f->pos + (long)want > f->cap)
        return 0;

    for (i = 0; i < want; i++)
        f->wr[f->pos + (long)i] = s[i];
    f->pos += (long)want;
    if (f->pos > f->len)
        f->len = f->pos;
    return count;
}

int getc(FILE *f)
{
    if (!f || !f->rd || f->pos >= f->len)
        return -1;
    return f->rd[f->pos++];
}

int putchar(int c)
{
    unsigned char b = (unsigned char)c;
    fwrite(&b, 1, 1, stdout);
    return c;
}

int puts(const char *s)
{
    fwrite(s, 1, strlen(s), stdout);
    return putchar('\n');
}

int ferror(FILE *f)
{
    (void)f;
    return 0;
}

int setvbuf(FILE *f, char *buf, int mode, size_t size)
{
    (void)f;
    (void)buf;
    (void)mode;
    (void)size;
    return 0;
}

/*
 * The entry calls this once, as fprintf(stderr, "%s\n", message),
 * and only on the way to giving up.  Nothing else is understood.
 */
int fprintf(FILE *f, const char *fmt, ...)
{
    (void)f;
    (void)fmt;
    return 0;
}

/* ── string ───────────────────────────────────────────────── */

void *memcpy(void *d, const void *s, size_t n)
{
    unsigned char *a = (unsigned char *)d;
    const unsigned char *b = (const unsigned char *)s;
    size_t i;

    for (i = 0; i < n; i++)
        a[i] = b[i];
    return d;
}

void *memmove(void *d, const void *s, size_t n)
{
    unsigned char *a = (unsigned char *)d;
    const unsigned char *b = (const unsigned char *)s;
    size_t i;

    if (a < b) {
        for (i = 0; i < n; i++)
            a[i] = b[i];
    } else {
        for (i = n; i > 0; i--)
            a[i - 1] = b[i - 1];
    }
    return d;
}

void *memset(void *d, int c, size_t n)
{
    unsigned char *a = (unsigned char *)d;
    size_t i;

    for (i = 0; i < n; i++)
        a[i] = (unsigned char)c;
    return d;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;
    size_t i;

    for (i = 0; i < n; i++)
        if (x[i] != y[i])
            return x[i] < y[i] ? -1 : 1;
    return 0;
}

size_t strlen(const char *s)
{
    size_t n = 0;

    while (s[n])
        n++;
    return n;
}

char *strchr(const char *s, int c)
{
    for (;; s++) {
        if (*s == (char)c)
            return (char *)s;
        if (!*s)
            return 0;
    }
}
