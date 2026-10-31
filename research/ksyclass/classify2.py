import sys, os, re, ast, yaml, collections, json
ROOT = sys.argv[1]
KEYS = ("size", "repeat-expr", "pos")
MEASURED = {"_sizeof", "size", "length"}   # known after a write pass

def split_ternary(e):
    depth = 0
    for i, ch in enumerate(e):
        if ch in "([": depth += 1
        elif ch in ")]": depth -= 1
        elif ch == "?" and depth == 0:
            rest = e[i+1:]; d2 = 0; nest = 0
            for j, c2 in enumerate(rest):
                if c2 in "([": d2 += 1
                elif c2 in ")]": d2 -= 1
                elif c2 == "?" and d2 == 0: nest += 1
                elif c2 == ":" and d2 == 0:
                    if nest == 0: return e[:i], rest[:j], rest[j+1:]
                    nest -= 1
    return None

def norm(e):
    e = re.sub(r"\.as<[^>]*>", "", e)
    e = re.sub(r"sizeof<[^>]*>", "7", e)          # a constant
    e = e.replace("::", "__")
    e = re.sub(r"\btrue\b", "True", e); e = re.sub(r"\bfalse\b", "False", e)
    return e

def chain(n):
    parts = []
    while True:
        if isinstance(n, ast.Attribute): parts.append(n.attr); n = n.value
        elif isinstance(n, ast.Subscript):
            s = n.slice
            if isinstance(s, ast.Name) and s.id in ("_index", "i") or isinstance(s, ast.Constant):
                parts.append("[]"); n = n.value
            else: return None
        else: break
    if isinstance(n, ast.Name):
        parts.append(n.id); return ".".join(reversed(parts))
    return None

def classify(expr, values, depth=0):
    e = str(expr).strip()
    t3 = split_ternary(e)
    if t3:
        kb, _ = classify(t3[1], values, depth); kc, _ = classify(t3[2], values, depth)
        ok = {"const","measured","field","affine","linear"}
        return ("piecewise" if kb in ok and kc in ok else "other"), []
    try: t = ast.parse(norm(e), mode="eval").body
    except SyntaxError: return "other", []
    refs, flags = [], set()
    def walk(n):
        if isinstance(n, ast.Constant) and isinstance(n.value, (int, float)): return "c"
        c = chain(n)
        if c is not None:
            parts = c.split(".")
            if "_io" in parts: flags.add("io"); return "v"
            if parts[-1] in MEASURED and len(parts) > 1: flags.add("measured"); return "c"
            if c in values and depth < 6:
                k, r = classify(values[c], values, depth + 1)
                if k in ("io",): flags.add("io")
                if k in ("other", "piecewise", "product"): flags.add(k)
                if k == "measured": flags.add("measured")
                refs.extend(r); return "c" if k in ("const","measured") else "v"
            refs.append(c); return "v"
        if isinstance(n, ast.UnaryOp) and isinstance(n.op, ast.USub): return walk(n.operand)
        if isinstance(n, ast.BinOp):
            a, b = walk(n.left), walk(n.right)
            if isinstance(n.op, (ast.Add, ast.Sub)): return "v" if "v" in (a, b) else "c"
            if isinstance(n.op, ast.Mult):
                if a == "v" and b == "v": flags.add("product")
                return "v" if "v" in (a, b) else "c"
            if isinstance(n.op, (ast.FloorDiv, ast.Div)):
                if b == "v": flags.add("other")
                return "v" if "v" in (a, b) else "c"
            flags.add("other"); return "v"
        flags.add("other")
        for ch in ast.iter_child_nodes(n): walk(ch)
        return "v"
    walk(t)
    if "other" in flags: return "other", refs
    if "io" in flags: return "io", refs
    if "product" in flags: return "product", refs
    if "piecewise" in flags: return "piecewise", refs
    if not refs: return ("measured" if "measured" in flags else "const"), refs
    if len(set(refs)) > 1: return "linear", refs
    if chain(t) is not None and "measured" not in flags: return "field", refs
    return "affine", refs

rows, files = [], []
def visit(path, tname, spec):
    values = {k: v["value"] for k, v in (spec.get("instances") or {}).items() if isinstance(v, dict) and "value" in v}
    attrs = [(a.get("id","?"), a) for a in (spec.get("seq") or []) if isinstance(a, dict)]
    attrs += [(k, v) for k, v in (spec.get("instances") or {}).items() if isinstance(v, dict) and "value" not in v]
    for aid, a in attrs:
        for key in KEYS:
            if key in a:
                k, r = classify(a[key], values)
                rows.append(dict(file=path, type=tname, attr=aid, key=key, expr=str(a[key]), kind=k))
    for sub, s in (spec.get("types") or {}).items():
        if isinstance(s, dict): visit(path, sub, s)
for dp, _, fs in os.walk(ROOT):
    if "/_build" in dp or "/.git" in dp: continue
    for f in fs:
        if f.endswith(".ksy"):
            p = os.path.join(dp, f); rp = os.path.relpath(p, ROOT); files.append(rp)
            visit(rp, "<root>", yaml.safe_load(open(p)))
json.dump(rows, open("rows2.json", "w"), indent=1)
K = ("const","measured","field","affine","linear","piecewise","product","io","other")
print(f"files {len(files)}  expressions {len(rows)}")
for key in KEYS + ("all",):
    sel = [r for r in rows if key == "all" or r["key"] == key]
    c = collections.Counter(r["kind"] for r in sel)
    print(f"{key:12} n={len(sel):5} " + " ".join(f"{k}={c[k]}" for k in K))
der = [r for r in rows if r["kind"] not in ("const","measured")]
c = collections.Counter(r["kind"] for r in der)
print(f"\nnon-trivial n={len(der)}: " + "  ".join(f"{k}={c[k]} ({100*c[k]/len(der):.0f}%)" for k in K[2:]))
GOOD = {"const","measured","field","affine","linear","piecewise"}
byf = collections.defaultdict(set)
for r in rows: byf[r["file"]].add(r["kind"])
n = len(files)
a = sum(1 for f in files if byf[f] <= GOOD)
b = sum(1 for f in files if byf[f] <= GOOD | {"io"})
c2 = sum(1 for f in files if byf[f] <= GOOD | {"io","product"})
z = sum(1 for f in files if not byf[f] - {"const","measured"})
print(f"formats: nothing to derive {z}/{n};  solvable by layout iteration {a}/{n} ({100*a/n:.0f}%);  +io {b}/{n} ({100*b/n:.0f}%);  +product-as-check {c2}/{n} ({100*c2/n:.0f}%)")
print("\nformats with 'other':", collections.Counter(r["file"] for r in rows if r["kind"]=="other").most_common(12))
