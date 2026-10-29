#!/usr/bin/env python3
# ══════════════════════════════════════════════════════════
#  What `make check` cannot see.
#
#  The checksums say a build came out as it did before.  They do
#  not say it was right before, and a change that is meant to move
#  bytes leaves them nothing to say at all.  This reads what was
#  built and checks it the way a reader would, then checks that the
#  order things are written in does not reach the file:
#
#    every dex      the checksum and the signature, the map against
#                   the header and against itself, each pool in the
#                   order the format wants, MUTF-8 and the lengths
#                   it claims, alignment, superclasses first;
#    every AXML     chunk sizes, the resource map ascending, each
#                   element's attributes by resource id and style=
#                   where the header says it is;
#    back again     the dex read back into source by disasm.py and
#                   assembled, which must give the same bytes;
#    order          the fields and methods of a class, the lists
#                   handed to dex_lists, the attributes of an
#                   element: reversed and shuffled, each must give
#                   the same bytes as the build.
#
#  Python 3 and nothing beyond its standard library.  It reads the
#  outputs of a build, so run it after one: `make verify`.
# ══════════════════════════════════════════════════════════

import hashlib, os, random, re, struct, subprocess, sys, zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FASMG = os.path.join(ROOT, 'deps', 'fasmg')
failures = []

def fail(what):
    failures.append(what)
    print('  FAIL', what)

# ── dex ───────────────────────────────────────────────────

def uleb(b, o):
    r = s = 0
    while True:
        x = b[o]; o += 1
        r |= (x & 0x7F) << s; s += 7
        if not x & 0x80:
            return r, o

def check_dex(path):
    b = open(path, 'rb').read()
    u = lambda f, o: struct.unpack_from(f, b, o)
    errs = []
    if b[:8] != b'dex\n035\0': errs.append('magic')
    if u('<I', 8)[0] != zlib.adler32(b[12:]): errs.append('adler32')
    if b[12:32] != hashlib.sha1(b[32:]).digest(): errs.append('signature')
    if u('<I', 32)[0] != len(b): errs.append('file_size')
    (sn, so, tn, to, pn, po, fn, fo, mn, mo, cn, co, dsz, doff) = u('<14I', 56)

    mapoff = u('<I', 52)[0]
    n = u('<I', mapoff)[0]
    items = [u('<HHII', mapoff + 4 + 12 * i) for i in range(n)]
    offs = [i[3] for i in items]
    if offs != sorted(offs): errs.append('map not in offset order')
    mp = {i[0]: (i[2], i[3]) for i in items}
    if len(mp) != n: errs.append('a map type twice')
    for t, cnt, off in ((1, sn, so), (2, tn, to), (3, pn, po), (4, fn, fo),
                        (5, mn, mo), (6, cn, co)):
        if cnt and mp.get(t) != (cnt, off): errs.append(f'map and header disagree on {t:#x}')
        if not cnt and (off or t in mp): errs.append(f'empty section {t:#x} listed')

    def string(i):
        o = u('<I', so + 4 * i)[0]
        n16, o = uleb(b, o)
        e = b.index(0, o)
        text = b[o:e].replace(b'\xc0\x80', b'\0').decode('utf-8', 'surrogatepass')
        if len(text.encode('utf-16-le', 'surrogatepass')) // 2 != n16:
            errs.append(f'string {i} claims {n16} UTF-16 units')
        return text
    strs = [string(i) for i in range(sn)]
    # by UTF-16 code unit, strictly: the order ART checks
    keys = [s.encode('utf-16-be', 'surrogatepass') for s in strs]
    if any(keys[i] >= keys[i + 1] for i in range(len(keys) - 1)): errs.append('strings out of order')

    tidx = [u('<I', to + 4 * i)[0] for i in range(tn)]
    if any(tidx[i] >= tidx[i + 1] for i in range(len(tidx) - 1)): errs.append('types out of order')
    types = [strs[i] for i in tidx]

    def tlist(o):
        if not o: return ()
        if o % 4: errs.append('type_list misaligned')
        return tuple(u('<H', o + 4 + 2 * j)[0] for j in range(u('<I', o)[0]))
    protos = [u('<III', po + 12 * i) for i in range(pn)]
    pkeys = [(p[1], tlist(p[2])) for p in protos]
    if any(pkeys[i] >= pkeys[i + 1] for i in range(len(pkeys) - 1)): errs.append('prototypes out of order')
    fields = [u('<HHI', fo + 8 * i) for i in range(fn)]
    fkeys = [(f[0], f[2], f[1]) for f in fields]
    if any(fkeys[i] >= fkeys[i + 1] for i in range(len(fkeys) - 1)): errs.append('fields out of order')
    methods = [u('<HHI', mo + 8 * i) for i in range(mn)]
    mkeys = [(m[0], m[2], m[1]) for m in methods]
    if any(mkeys[i] >= mkeys[i + 1] for i in range(len(mkeys) - 1)): errs.append('methods out of order')

    # superclasses and interfaces this file defines come first
    classes = {u('<I', co + 32 * i)[0] for i in range(cn)}
    defined, codes = set(), 0
    for i in range(cn):
        ci, fl, sup, ifs, src, ann, cd, sv = u('<8I', co + 32 * i)
        for dep in (sup,) + tlist(ifs):
            if dep in classes and dep not in defined:
                errs.append(f'{types[ci]} comes before {types[dep]}')
        defined.add(ci)
        if not cd: continue
        o, counts = cd, []
        for _ in range(4):
            c, o = uleb(b, o); counts.append(c)
        for k, c in enumerate(counts):
            for _ in range(c):
                _, o = uleb(b, o)                # index, as a difference
                _, o = uleb(b, o)                # flags
                if k >= 2:
                    off, o = uleb(b, o)          # code
                    if off:
                        codes += 1
                        if off % 4: errs.append('code_item misaligned')
    if mp.get(0x2001, (0,))[0] != codes: errs.append('map counts the code items wrong')
    return errs

# ── AXML ──────────────────────────────────────────────────

def check_axml(path):
    b = open(path, 'rb').read()
    u = lambda f, o: struct.unpack_from(f, b, o)
    errs = []
    t, hs, size = u('<HHI', 0)
    if t != 0x0003 or size != len(b): return ['file chunk']
    o, strings, resmap = hs, [], []
    while o < len(b):
        t, hs, sz = u('<HHI', o)
        if sz < 8 or o + sz > len(b): return errs + ['chunk size']
        if t == 0x0001:
            n, _, flags, start, _ = u('<5I', o + 8)
            for i in range(n):
                p = o + start + u('<I', o + hs + 4 * i)[0]
                p += 2 if b[p] & 0x80 else 1
                l = b[p]; p += 1
                if l & 0x80: l = ((l & 0x7F) << 8) | b[p]; p += 1
                strings.append(b[p:p + l].decode())
        elif t == 0x0180:
            resmap = [u('<I', o + 8 + 4 * i)[0] for i in range((sz - 8) // 4)]
            if resmap != sorted(resmap): errs.append('resource map out of order')
        elif t == 0x0102:
            ns, name, ast, asz, ac, idi, cli, sti = u('<IIHHHHHH', o + 16)
            if sz != 0x24 + asz * ac: errs.append('element size')
            names, ids = [], []
            for i in range(ac):
                an = u('<I', o + 16 + ast + asz * i + 4)[0]
                names.append(strings[an])
                ids.append(resmap[an] if an < len(resmap) else 0)
            with_id = [x for x in ids if x]
            if ids[:len(with_id)] != with_id or with_id != sorted(with_id):
                errs.append(f'<{strings[name]}> attributes out of order')
            if 'style' in names and (not sti or names[sti - 1] != 'style'):
                errs.append(f'<{strings[name]}> style= not where the header says')
        o += sz
    return errs

# ── order ─────────────────────────────────────────────────

def assemble(src_dir, text, out):
    # The copy sits beside the original so that its includes resolve.
    tmp = os.path.join(src_dir, f'.verify-{os.getpid()}.asm')
    try:
        open(tmp, 'w').write(text)
        r = subprocess.run([FASMG, os.path.basename(tmp), out], cwd=src_dir,
                           capture_output=True, text=True, timeout=600)
        return r.returncode == 0
    finally:
        os.remove(tmp)

DECL = re.compile(r'^    (deffield|method|defclass|deftype)\b')

def permute_members(lines, kind, rng):
    """Reorder the declarations of one kind inside each macro body."""
    out, body, inmac = [], [], False
    for l in lines:
        if l.startswith('macro _') and not inmac:
            inmac, body = True, []
            out.append(l); continue
        if inmac and l.startswith('end macro'):
            units, cur = [], None
            for x in body:
                m = DECL.match(x)
                if m:
                    cur = [m.group(1), [x]]; units.append(cur)
                elif cur is None:
                    units.append(['', [x]])
                else:
                    cur[1].append(x)
            idx = [i for i, un in enumerate(units) if un[0] == kind]
            sel = [units[i] for i in idx]
            sel = sel[::-1] if rng is None else rng.sample(sel, len(sel))
            for i, un in zip(idx, sel): units[i] = un
            for un in units: out.extend(un[1])
            out.append(l); inmac = False; continue
        (body if inmac else out).append(l)
    return out

def permute_attrs(lines, rng):
    out, run = [], []
    def flush():
        out.extend(run[::-1] if rng is None else rng.sample(run, len(run)))
        run.clear()
    for l in lines:
        if re.match(r'\s+attr_\w+', l) and not l.rstrip().endswith('\\'):
            run.append(l)
        else:
            flush(); out.append(l)
    flush()
    return out

def permute_lists(text, rng):
    m = re.search(r'^dex_lists (.*)$', text, re.M)
    if not m: return text
    args = [a.strip() for a in m.group(1).split(',')]
    args = args[::-1] if rng is None else rng.sample(args, len(args))
    return text.replace(m.group(0), 'dex_lists ' + ', '.join(args))

def build_vars(d):
    v = {}
    for l in open(os.path.join(d, 'build.sh')):
        m = re.match(r'(\w+)=("?)(.*)\2$', l.strip())
        if m: v[m.group(1)] = m.group(3)
    return v

def check_order(ex, d, tmpdir):
    v = build_vars(d)
    cases = []
    asm = open(os.path.join(d, v['ASM'])).read()
    lines = asm.split('\n')
    for how in (None, random.Random(ex)):
        for kind in ('deffield', 'method'):
            cases.append((f'{kind}s', v['ASM'], '\n'.join(permute_members(lines, kind, how)), 'classes.dex'))
        cases.append(('dex_lists', v['ASM'], permute_lists(asm, how), 'classes.dex'))
        sources = [(v.get('MANIFEST', 'manifest.asm'), 'AndroidManifest.xml')]
        for lay in v.get('LAYOUT_ASM', '').split():
            sources.append((lay, os.path.join('res', 'layout', lay[:-4] + '.xml')))
        for src, built in sources:
            text = open(os.path.join(d, src)).read()
            cases.append((f'attributes in {src}', src, '\n'.join(permute_attrs(text.split('\n'), how)), built))
    tried = 0
    for what, src, text, built in cases:
        if text == open(os.path.join(d, src)).read(): continue
        tried += 1
        out = os.path.join(tmpdir, 'out')
        if not assemble(d, text, out):
            fail(f'{ex}: {what} reordered does not assemble'); continue
        if open(out, 'rb').read() != open(os.path.join(d, built), 'rb').read():
            fail(f'{ex}: {what} reordered changes {built}')
    return tried

# ── back again ────────────────────────────────────────────

def check_round_trip(ex, d, tmpdir):
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import disasm
    dex = os.path.join(d, 'classes.dex')
    src = os.path.join(tmpdir, 'back.asm')
    out = os.path.join(tmpdir, 'back.dex')
    open(src, 'w').write(disasm.disasm(dex))
    r = subprocess.run([FASMG, src, out], capture_output=True, text=True, timeout=600)
    if r.returncode:
        fail(f'{ex}: its source read back does not assemble')
    elif open(out, 'rb').read() != open(dex, 'rb').read():
        fail(f'{ex}: its source read back assembles to other bytes')

# ── all of it ─────────────────────────────────────────────

def main():
    import tempfile
    exdir = os.path.join(ROOT, 'examples')
    examples = sorted(e for e in os.listdir(exdir)
                      if os.path.exists(os.path.join(exdir, e, 'build.sh'))
                      and 'ASM' in build_vars(os.path.join(exdir, e)))
    with tempfile.TemporaryDirectory() as tmpdir:
        for ex in examples:
            d = os.path.join(exdir, ex)
            if not os.path.exists(os.path.join(d, 'classes.dex')):
                fail(f'{ex}: not built; run make all first'); continue
            for e in check_dex(os.path.join(d, 'classes.dex')):
                fail(f'{ex}/classes.dex: {e}')
            axml = [os.path.join(d, 'AndroidManifest.xml')]
            lay = os.path.join(d, 'res', 'layout')
            if os.path.isdir(lay):
                axml += [os.path.join(lay, f) for f in sorted(os.listdir(lay)) if f.endswith('.xml')]
            for p in axml:
                for e in check_axml(p):
                    fail(f'{os.path.relpath(p, exdir)}: {e}')
            check_round_trip(ex, d, tmpdir)
            n = check_order(ex, d, tmpdir)
            print(f'{ex:20} {len(axml)} AXML, {n} reorderings')
    print(f'{len(failures)} failures' if failures else 'verified')
    sys.exit(1 if failures else 0)

if __name__ == '__main__':
    main()
