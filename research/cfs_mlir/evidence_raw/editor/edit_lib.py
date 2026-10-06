import sys,re
def load(n): return open(f'work/{n}').read()
def save(n,t): open(f'work/{n}','w').write(t)
def rep(t, old, new, count=1):
    c=t.count(old)
    if c!=count:
        raise SystemExit(f"COUNT {c} != {count} for: {old[:120]!r}")
    return t.replace(old,new)
def rep_between(t, start, end, new, include_end=False):
    i=t.find(start); assert i>=0, ('start not found',start[:80])
    j=t.find(end, i+len(start)); assert j>=0, ('end not found',end[:80])
    if include_end: j+=len(end)
    return t[:i]+new+t[j:]
