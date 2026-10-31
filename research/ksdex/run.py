import sys, os, glob, inspect, traceback, collections
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), sys.argv[2]))
from kaitaistruct import KaitaiStruct
from dex import Dex

def props(o):
    names = []
    for cls in type(o).__mro__:
        for k, v in vars(cls).items():
            if isinstance(v, property) and not k.startswith("_"): names.append(k)
    return names

def force(o, path, errs, seen, depth=0):
    if id(o) in seen or depth > 40: return 0
    seen.add(id(o)); n = 1
    fields = [k for k in vars(o) if not k.startswith("_") and not k.startswith("m_")] + props(o)
    for k in fields:
        try: v = getattr(o, k)
        except Exception as e:
            errs.append((f"{path}.{k}", f"{type(e).__name__}: {e}")); continue
        for i, x in enumerate(v if isinstance(v, list) else [v]):
            if isinstance(x, KaitaiStruct): n += force(x, f"{path}.{k}" + (f"[{i}]" if isinstance(v, list) else ""), errs, seen, depth+1)
    return n

for f in sorted(glob.glob(sys.argv[1] + "/examples/*/classes.dex")):
    name = f.split("/")[-2]; data = open(f, "rb").read()
    errs = []
    try:
        d = Dex.from_bytes(data)
        nobj = force(d, "dex", errs, set())
    except Exception as e:
        print(f"{name:20} PARSE FAILED {type(e).__name__}: {e}"); continue
    h = d.header
    strs = []
    for s in d.string_ids:
        try: strs.append(s.value.data)
        except Exception as e: strs.append(None)
    bad = sum(x is None for x in strs); strs = [x for x in strs if x is not None]
    ok_sorted = all(a.encode('utf-16-be') < b.encode('utf-16-be') for a, b in zip(strs, strs[1:]))
    classes = [d.type_ids[c.class_idx].type_name for c in d.class_defs]
    print(f"{name:20} objs={nobj:5} errs={len(errs):3} file_size={h.file_size}=={len(data)}:{h.file_size==len(data)} "
          f"strings={len(strs)}+{bad}bad sorted(utf16)={ok_sorted} methods={len(d.method_ids)} map={len(d.map.list)} classes={classes}")
    for p, e in collections.OrderedDict(errs).items() if errs else []:
        pass
    seen = collections.Counter(e for _, e in errs)
    for e, c in seen.most_common(4): print(f"      {c}x {e[:150]}   e.g. {next(p for p,x in errs if x==e)}")
