#!/usr/bin/env python3
# ══════════════════════════════════════════════════════════
#  A dex, read back into forgedex's own source.
#
#      python3 tools/disasm.py classes.dex > back.asm
#
#  The instruction set is not written out here a second time.  It is
#  read from forge/dex-insn.inc, the table the assembler is made of:
#  every `def_23x add_int, $90` line names an opcode, a format and an
#  instruction, and a macro such as new_instance names its opcode in
#  the format call inside it.  What this file knows is what each kind
#  of helper takes — a register, a literal, a label, a reference —
#  which is some twenty-five shapes, not two hundred opcodes.
#
#  Every string, type, prototype, field and method in the file is
#  declared, whether the code names it or not, so the pools come
#  back whole; the order they are written in does not matter, since
#  forgedex puts every pool in its own order.  So assembling what
#  this prints gives the same bytes, for a dex that forgedex can
#  write — which make verify checks for every example.
#
#  Not read: annotations, debug information, static values, a try
#  range that only part of another covers.  Each is an error here
#  rather than something left out in silence.
# ══════════════════════════════════════════════════════════

import os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ── The table, as the assembler has it ───────────────────

# What a helper's operands are: r a register, n a literal, L a label,
# F a field, M a method, T a type, S a string, * a register list,
# . a register range.
HELPERS = {
    'def_10x': ('10x', ''),    'def_10t': ('10t', 'L'),   'def_20t': ('20t', 'L'),
    'def_30t': ('30t', 'L'),   'def_11x': ('11x', 'r'),   'def_11x_w': ('11x', 'r'),
    'def_11n': ('11n', 'rn'),  'def_12x': ('12x', 'rr'),  'def_12x_ww': ('12x', 'rr'),
    'def_12x_wn': ('12x', 'rr'), 'def_12x_nw': ('12x', 'rr'),
    'def_21c_ref': ('21c', 'rF'), 'def_21c_ref_w': ('21c', 'rF'),
    'def_21t': ('21t', 'rL'),  'def_21s': ('21s', 'rn'),
    'def_22c_ref': ('22c', 'rrF'), 'def_22c_ref_w': ('22c', 'rrF'),
    'def_22t': ('22t', 'rrL'), 'def_22b': ('22b', 'rrn'), 'def_22s': ('22s', 'rrn'),
    'def_22x': ('22x', 'rr'),  'def_22x_ww': ('22x', 'rr'),
    'def_23x': ('23x', 'rrr'), 'def_23x_www': ('23x', 'rrr'), 'def_23x_wnn': ('23x', 'rrr'),
    'def_23x_wwn': ('23x', 'rrr'), 'def_23x_nww': ('23x', 'rrr'),
    'def_31i': ('31i', 'rn'),  'def_51l': ('51l', 'rn'),
    'def_32x': ('32x', 'rr'),  'def_32x_ww': ('32x', 'rr'),
    'def_35c_ref': ('35c', 'M*'), 'def_3rc_ref': ('3rc', 'M.'),
}

# A parameter's name says what it is, in the macros written out in full.
PARAM = {'a': 'r', 'b': 'r', 'lit': 'n', 'text': 'S', 'descriptor': 'T',
         'regs': '*', 'first': '.', 'last': ''}

def read_table():
    src = open(os.path.join(ROOT, 'forge', 'dex-insn.inc')).read()
    ops = {}
    for helper, name, op in re.findall(r'^(def_\w+)\s+(\w+)\s*,\s*\$([0-9A-Fa-f]{2})', src, re.M):
        ops.setdefault(int(op, 16), (name,) + HELPERS[helper])
    for name, params, body in re.findall(r'^macro (\w+)\?\s*([^\n]*)\n(.*?)^end macro', src, re.M | re.S):
        m = re.search(r'(__fmt_(\w+)|_pl_31t)\s+\$([0-9A-Fa-f]{2})(?:\s*,[^,\n]*,[^,\n]*,\s*\$([0-9A-Fa-f]+))?', body)
        if not m:
            continue
        op = int(m.group(3), 16)
        if op in ops:
            continue                  # the table line already says it
        names = [p.strip().rstrip('?*&').rstrip('?') for p in params.split(',') if p.strip()]
        if m.group(1) == '_pl_31t':
            ops[op] = (name, '31t', {0x2B: 'P', 0x2C: 'Q', 0x26: 'A'}[op])
        elif m.group(2) == '21h':
            ops[op] = (name, '21h' + str(int(m.group(4), 16)), 'rn')
        else:
            ops[op] = (name, m.group(2), ''.join(PARAM[n] for n in names))
    return ops

# ── The file ─────────────────────────────────────────────

def uleb(b, o):
    r = s = 0
    while True:
        x = b[o]; o += 1
        r |= (x & 0x7F) << s; s += 7
        if not x & 0x80:
            return r, o

def sleb(b, o):
    r = s = 0
    while True:
        x = b[o]; o += 1
        r |= (x & 0x7F) << s; s += 7
        if not x & 0x80:
            return (r - (1 << s) if x & 0x40 else r), o

def s(x, bits):
    return x - (1 << bits) if x & (1 << (bits - 1)) else x

def quote(text):
    if any(ord(c) < 0x20 for c in text):
        raise SystemExit(f'disasm: a string with a control character cannot be written: {text!r}')
    return '"' + text.replace('"', '""') + '"'

class Dex:
    def __init__(self, b):
        self.b = b
        u = self.u
        (sn, so, tn, to, pn, po, fn, fo, mn, mo, cn, co) = u('<12I', 56)
        def string(i):
            o = u('<I', so + 4 * i)[0]
            _, o = uleb(b, o)
            return b[o:b.index(0, o)].replace(b'\xc0\x80', b'\0').decode('utf-8', 'surrogatepass')
        self.strings = [string(i) for i in range(sn)]
        self.types = [self.strings[u('<I', to + 4 * i)[0]] for i in range(tn)]
        self.protos, self.proto_lists = [], {}
        for i in range(pn):
            sh, rt, pl = u('<III', po + 12 * i)
            self.protos.append((self.types[rt], self.tlist(pl)))
            if pl:
                self.proto_lists.setdefault(pl, i)
        self.fields = [u('<HHI', fo + 8 * i) for i in range(fn)]
        self.methods = [u('<HHI', mo + 8 * i) for i in range(mn)]
        self.classes = [u('<8I', co + 32 * i) for i in range(cn)]

    def u(self, f, o):
        return struct.unpack_from(f, self.b, o)

    def tlist(self, o):
        if not o:
            return ()
        return tuple(self.types[self.u('<H', o + 4 + 2 * j)[0]] for j in range(self.u('<I', o)[0]))

    def field(self, i):
        c, t, n = self.fields[i]
        return f'{self.types[c]}->{self.strings[n]}:{self.types[t]}'

    def method(self, i):
        c, p, n = self.methods[i]
        ret, params = self.protos[p]
        return f'{self.types[c]}->{self.strings[n]}({"".join(params)}){ret}'

# ── Code ──────────────────────────────────────────────────

# Registers have names up to v255; one past them is written from v0,
# the way the assembler takes it.
def reg(n):
    return f'v{n}' if n < 256 else f'v0 + {n}'

def code(dex, ops, off):
    u = dex.u
    regs, ins, outs, tries, dbg, n = u('<HHHHII', off)
    if dbg:
        raise SystemExit('disasm: debug information is not read')
    base = off + 16
    w = lambda a: u('<H', base + 2 * a)[0]
    labels, payloads, lines = set(), {}, []
    # first walk: where instructions are, where payloads are
    a, starts = 0, []
    while a < n:
        if a in payloads:
            a = payloads[a]; continue
        op = w(a) & 0xFF
        if op not in ops:
            raise SystemExit(f'disasm: opcode {op:#04x} is not in forge/dex-insn.inc')
        name, fmt, kinds = ops[op]
        starts.append(a)
        size = {'10x': 1, '10t': 1, '11x': 1, '11n': 1, '12x': 1, '20t': 2, '21c': 2, '21t': 2,
                '21s': 2, '21h16': 2, '21h48': 2, '22c': 2, '22t': 2, '22b': 2, '22s': 2,
                '22x': 2, '23x': 2, '30t': 3, '31i': 3, '31c': 3, '31t': 3, '32x': 3,
                '35c': 3, '3rc': 3, '51l': 5}[fmt]
        if fmt == '31t':
            pl = a + s(w(a + 1) | w(a + 2) << 16, 32)
            ident, width = w(pl), w(pl + 1)
            if ident == 0x0100:
                end = pl + 4 + 2 * width
            elif ident == 0x0200:
                end = pl + 2 + 4 * width
            else:
                cnt = w(pl + 2) | w(pl + 3) << 16
                end = pl + 4 + (width * cnt + 1) // 2
            payloads[pl] = end
        a += size
    # the payloads are laid out again by the assembler, with the nops
    # that align them, so neither is an instruction here
    first_payload = min(payloads) if payloads else n
    starts = [x for x in starts if x < first_payload and not (
        w(x) == 0 and all(w(y) == 0 for y in range(x, first_payload)))]
    def label(t):
        labels.add(t); return f'.L{t}'
    for a in starts:
        name, fmt, kinds = ops[w(a) & 0xFF]
        A, B = w(a) >> 8, w(a + 1) if a + 1 < n else 0
        out = []
        if fmt == '31t':
            pl = a + s(w(a + 1) | w(a + 2) << 16, 32)
            size = w(pl + 1)
            if kinds == 'P':
                first = s(w(pl + 2) | w(pl + 3) << 16, 32)
                tg = [s(w(pl + 4 + 2 * i) | w(pl + 5 + 2 * i) << 16, 32) for i in range(size)]
                out = [reg(A), str(first)] + [label(a + t) for t in tg]
            elif kinds == 'Q':
                keys = [s(w(pl + 2 + 2 * i) | w(pl + 3 + 2 * i) << 16, 32) for i in range(size)]
                tg = [s(w(pl + 2 + 2 * size + 2 * i) | w(pl + 3 + 2 * size + 2 * i) << 16, 32) for i in range(size)]
                out = [reg(A)] + [x for k, t in zip(keys, tg) for x in (str(k), label(a + t))]
            else:
                cnt = w(pl + 2) | w(pl + 3) << 16
                raw = dex.b[base + 2 * (pl + 4): base + 2 * (pl + 4) + size * cnt]
                vals = [int.from_bytes(raw[i * size:(i + 1) * size], 'little', signed=True) for i in range(cnt)]
                out = [reg(A), str(size)] + [str(v) for v in vals]
            lines.append((a, name, out)); continue
        # the operands, by format, in the order the macro takes them
        if fmt == '10x': vals = []
        elif fmt == '10t': vals = [a + s(A, 8)]
        elif fmt == '20t': vals = [a + s(B, 16)]
        elif fmt == '30t': vals = [a + s(w(a + 1) | w(a + 2) << 16, 32)]
        elif fmt == '11x': vals = [A]
        elif fmt == '11n': vals = [A & 15, s(A >> 4, 4)]
        elif fmt == '12x': vals = [A & 15, A >> 4]
        elif fmt in ('21c', '21s', '21t', '22x'):
            vals = [A, s(B, 16) if fmt in ('21s', '21t') else B]
            if fmt == '21t': vals[1] += a
        elif fmt == '21h16': vals = [A, B << 16]
        elif fmt == '21h48': vals = [A, B << 48]
        elif fmt in ('22c', '22t', '22s'):
            vals = [A & 15, A >> 4, s(B, 16) if fmt != '22c' else B]
            if fmt == '22t': vals[2] += a
        elif fmt == '22b': vals = [A, B & 0xFF, s(B >> 8, 8)]
        elif fmt == '23x': vals = [A, B & 0xFF, B >> 8]
        elif fmt == '31i': vals = [A, s(B | w(a + 2) << 16, 32)]
        elif fmt == '31c': vals = [A, B | w(a + 2) << 16]
        elif fmt == '32x': vals = [B, w(a + 2)]
        elif fmt == '51l': vals = [A, s(sum(w(a + 1 + i) << 16 * i for i in range(4)), 64)]
        elif fmt == '35c':
            g = w(a + 2); cnt = A >> 4
            vals = [B, [g & 15, g >> 4 & 15, g >> 8 & 15, g >> 12, A & 15][:cnt]]
        elif fmt == '3rc':
            vals = [B, w(a + 2), w(a + 2) + A - 1]
        # the operands, by what each one is
        for k, v in zip(kinds, vals):
            if k == 'r': out.append(reg(v))
            elif k == 'n': out.append(str(v))
            elif k == 'L': out.append(label(v))
            elif k == 'F': out.append(quote(dex.field(v)))
            elif k == 'M': out.append(quote(dex.method(v)))
            elif k == 'T': out.append(quote(dex.types[v]))
            elif k == 'S': out.append(quote(dex.strings[v]))
            elif k == '*': out += [reg(x) for x in v]
            elif k == '.': out += [reg(v), reg(vals[2])]
        if fmt == '3rc' and kinds == 'T.':
            out = out[:3]
        lines.append((a, name, out))
    # tries
    catches = []
    if tries:
        o = base + 2 * n + (2 if n % 2 else 0)
        hl = o + 8 * tries
        for i in range(tries):
            start, cnt, hoff = u('<IHH', o + 8 * i)
            q = hl + hoff
            size, q = sleb(dex.b, q)
            for _ in range(abs(size)):
                t, q = uleb(dex.b, q); h, q = uleb(dex.b, q)
                catches.append(f'catch {quote(dex.types[t])}, {label(start)}, {label(start + cnt)}, {label(h)}')
            if size <= 0:
                h, q = uleb(dex.b, q)
                catches.append(f'catch_all {label(start)}, {label(start + cnt)}, {label(h)}')
    body = []
    for a, name, out in lines:
        if a in labels: body.append(f'    .L{a}:')
        body.append(f'        {name} {", ".join(out)}'.rstrip())
    end = first_payload
    if end in labels: body.append(f'    .L{end}:')
    body += [f'        {c}' for c in catches]
    return regs, ins, body

# ── The source ────────────────────────────────────────────

def disasm(path):
    b = open(path, 'rb').read()
    dex, ops = Dex(b), read_table()
    out = ['format binary as "dex"', 'org $00', '',
           f"include '{os.path.join(ROOT, 'forgedex.inc')}'", '']
    members_f, members_m, ifaces = set(), set(), []
    classes = []
    for k, (ci, fl, sup, ifs, src, ann, cd, sv) in enumerate(dex.classes):
        if ann or sv:
            raise SystemExit('disasm: annotations and static values are not read')
        c = [f'macro _class{k}',
             f'    defclass {quote(dex.types[ci])}, ${fl:X}, '
             f'{quote(dex.types[sup]) if sup != 0xFFFFFFFF else "0"}' +
             (f', _ifaces{k}' if ifs else '')]
        #  A class may point its interfaces at a prototype's own list,
        #  as `_hello_ifaces = _oncl_tl` does, and then it shares it.
        if ifs in dex.proto_lists:
            ifaces.append(f'    _ifaces{k} = _p{dex.proto_lists[ifs]}_tl')
        elif ifs:
            ifaces.append(f'    _ifaces{k} deftypelist ' + ', '.join(quote(t) for t in dex.tlist(ifs)))
        if cd:
            o = cd
            counts = []
            for _ in range(4):
                x, o = uleb(b, o); counts.append(x)
            for lst, cnt in enumerate(counts):
                idx = 0
                for j in range(cnt):
                    d, o = uleb(b, o); idx = d if j == 0 else idx + d
                    flags, o = uleb(b, o)
                    if lst < 2:
                        members_f.add(idx)
                        c.append(f'    deffield {quote(dex.field(idx))}, ${flags:X}')
                        continue
                    coff, o = uleb(b, o)
                    members_m.add(idx)
                    if not coff:
                        c.append(f'    defmethod {quote(dex.method(idx))}, ${flags:X}')
                        continue
                    regs, ins, body = code(dex, ops, coff)
                    c.append(f'    method {quote(dex.method(idx))}, ${flags:X}, locals {regs - ins}')
                    c += body
                    c.append('    end method')
        c.append('end macro')
        classes.append('\n'.join(c))
    out += classes
    # the pools whole: everything declared, whatever names it
    decl = ['macro _pools']
    decl += [f'    defstr _s{i}, {quote(t)}' for i, t in enumerate(dex.strings)]
    decl += [f'    deftype {quote(t)}' for t in dex.types]
    for i, (ret, params) in enumerate(dex.protos):
        decl.append(f'    defproto _p{i}, {quote(ret)}, <{", ".join(quote(p) for p in params)}>')
    decl += [f'    deffield {quote(dex.field(i))}' for i in range(len(dex.fields)) if i not in members_f]
    decl += [f'    defmethod {quote(dex.method(i))}' for i in range(len(dex.methods)) if i not in members_m]
    decl.append('end macro')
    out += ['', '\n'.join(decl), '', 'macro _interfaces'] + ifaces + ['end macro', '']
    out.append('dex_lists ' + ', '.join([f'_class{k}' for k in range(len(classes))] + ['_pools']))
    out.append('dex_file _interfaces')
    return '\n'.join(out) + '\n'

if __name__ == '__main__':
    sys.stdout.write(disasm(sys.argv[1]))
