#!/usr/bin/env python3
# semantics_scan.py -- the mechanical scan of the semantics coverage rule (kairos/docs/semantics_coverage.md), the generator
# of the semantics data files under kairos/semantics/ and their provenance check.
#
#   python3 -I semantics_scan.py scan     <trees.tsv> <p0.cpp>             every entry the rule finds, with file:line
#   python3 -I semantics_scan.py generate <trees.tsv> <p0.cpp> <out dir>   write the data files (new files only)
#   python3 -I semantics_scan.py check    <trees.tsv> <p0.cpp> <data dir>  rc 0 when every data file equals what the scan
#                                                                          generates now and every pinned file and line holds
#
# trees.tsv: one line per component version, tab-separated key=value fields (the keys are listed in its head comment).
# p0.cpp: the P0 tables of kz-static (kairos/lib/llvm/p0.cpp); the names of their entries are read from its text.
#
# The rule (the document above is the authority; this is its implementation):
#   groups      the status, event-type and run-status constants: every integer macro (#define NAME value) or enumerator
#               (NAME = value) in the anchor's line range, with the value of its literal as written (a cast is not
#               applied) and its line
#   entries     every function declared in the public headers of the component (a prototype statement outside comments)
#               such that (E1) its return type or a parameter type is a status-type, event-type-type or run-status-type
#               typedef (or a pointer to one), or (E2) the comment block right above the declaration names a status
#               constant in a return line (@retval, \retval, \retcode, @return, \return, \returns) or an event-type or
#               run-status constant in the paragraph of a parameter (@param, \param, with its \arg lines), or (E3) its
#               definition in the component's sources returns a status constant (return NAME;)
#   namespaces  (N, the object-id generation: a component with the objid anchor) per object kind of the kind macro of a
#               source file: the lookup, create and delete functions of the file (their calls of the object-id layer with
#               the kind), the stored name (the init macro's field of the create's record and its character-array
#               declaration), the comparison function (a function comparing its parameter with the name field through a
#               byte-string comparison of the P0 comparison table), other stores into the name field, and the status
#               constants named in the bodies of the object-id lookup and the functions it calls in its file;
#               (T, the table generation: every other framework component) per stored-name path: the public functions
#               that compare a parameter with a subscripted table path through a byte-string comparison (lookup when the
#               function does not copy into that path, create when it copies a parameter into it), the functions that copy
#               a parameter into the path or clear it, and the extent of the path's last field where its declaration is
#               found through the declared types of the path
#   p0          (layer p0) for every entry name of the P0 tables, its declaration in the listed headers (the first
#               prototype statement of that name) and the attributes the declaration carries (pure, const, noreturn,
#               nonnull positions, warn-unused-result, nothrow); the lines of the local copy of C library source text
#               the P0 table cites (p0-source) are pinned by its sha256
# The scan reads files and prints; it opens nothing else and writes only under the output directory of `generate`.
import glob
import hashlib
import os
import re
import sys

RULE_REVISION = 'semantics coverage rule revision 1 (2026-11-16)'
FORMAT = 'semantics-data 1'
P0_COMPARE_FAMILY = ('byte-string comparison',)
WIDTHS = {'int8_t': (8, True), 'uint8_t': (8, False), 'int16_t': (16, True), 'uint16_t': (16, False), 'int32_t': (32, True),
          'uint32_t': (32, False), 'int64_t': (64, True), 'uint64_t': (64, False), 'signed char': (8, True), 'unsigned char': (8, False),
          'char': (8, True), 'short': (16, True), 'short int': (16, True), 'signed short': (16, True), 'unsigned short': (16, False),
          'unsigned short int': (16, False), 'int': (32, True), 'signed int': (32, True), 'signed': (32, True), 'unsigned int': (32, False),
          'unsigned': (32, False), 'long long': (64, True), 'long long int': (64, True), 'unsigned long long': (64, False),
          'unsigned long long int': (64, False)}


def sha(path):
    return hashlib.sha256(open(path, 'rb').read()).hexdigest()


# ---------------------------------------------------------------- input
def read_trees(path):
    comps = []
    for line in open(path):
        line = line.rstrip('\n')
        if not line or line.startswith('#'):
            continue
        d = {}
        for f in line.split('\t'):
            k, _, v = f.partition('=')
            d[k] = v
        comps.append(d)
    return comps


def p0_names(p0cpp):
    text = open(p0cpp).read()
    names = []
    for m in re.finditer(r'make\(KZ_P0_KIND_[A-Z0-9_]+, "[^"]*", "([A-Za-z_][A-Za-z0-9_]*)"', text):
        names.append((m.group(1), 'table'))
    for m in re.finditer(r'\bdata\("([A-Za-z_][A-Za-z0-9_]*)"', text):
        names.append((m.group(1), 'table'))
    for m in re.finditer(r'\{"([A-Za-z_][A-Za-z0-9_]*)", "((?:byte-string|thread-id) comparison)"', text):
        names.append((m.group(1), 'compare'))
    seen, out = set(), []
    for n, t in names:
        if n not in seen:
            seen.add(n)
            out.append((n, t))
    return out


# ---------------------------------------------------------------- C text
class Src:
    """A C file without its comments (replaced by spaces, newlines kept), with its comments by position."""

    def __init__(self, root, rel):
        self.root, self.rel = root, rel
        self.path = os.path.join(root, rel)
        raw = open(self.path, errors='replace').read()
        self.raw_lines = raw.split('\n')
        out = []
        comments = []  # (start line, end line, text)
        i, n, line = 0, len(raw), 1
        while i < n:
            c = raw[i]
            if c == '/' and i + 1 < n and raw[i + 1] == '*':
                j = raw.find('*/', i + 2)
                j = n if j < 0 else j + 2
                body = raw[i:j]
                comments.append((line, line + body.count('\n'), body))
                out.append(''.join('\n' if ch == '\n' else ' ' for ch in body))
                line += body.count('\n')
                i = j
                continue
            if c == '/' and i + 1 < n and raw[i + 1] == '/':
                j = raw.find('\n', i)
                j = n if j < 0 else j
                comments.append((line, line, raw[i:j]))
                out.append(' ' * (j - i))
                i = j
                continue
            if c in '"\'':
                j = i + 1
                while j < n and raw[j] != c:
                    if raw[j] == '\\':
                        j += 1
                    if j < n and raw[j] == '\n':
                        break
                    j += 1
                seg = raw[i:min(j + 1, n)]
                out.append(seg)
                line += seg.count('\n')
                i = min(j + 1, n)
                continue
            if c == '\n':
                line += 1
            out.append(c)
            i += 1
        self.code = ''.join(out)
        self.lines = self.code.split('\n')
        # the code without its preprocessor lines (blanked, offsets kept): the statements are read from it
        np_lines, cont = [], False
        for l in self.lines:
            if cont or l.lstrip().startswith('#'):
                cont = l.rstrip().endswith('\\')
                np_lines.append(' ' * len(l))
            else:
                np_lines.append(l)
        self.code_np = '\n'.join(np_lines)
        self.comments = comments
        self.sha256 = sha(self.path)
        self._line_at = None

    def line_of(self, pos):
        return self.code.count('\n', 0, pos) + 1

    def statements(self):
        """(text, start offset, end char, end offset) of each top-level statement: ends at ';', '{' or '}' outside parens,
        preprocessor lines left out; '{' blocks are skipped to their matching '}' (their offsets kept)."""
        code, n = self.code_np, len(self.code_np)
        i, start, depth = 0, 0, 0
        out = []
        while i < n:
            c = code[i]
            if c in '"\'':
                j = i + 1
                while j < n and code[j] != c and code[j] != '\n':
                    j += 2 if code[j] == '\\' else 1
                i = j + 1
                continue
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            elif depth == 0 and c in ';{}':
                out.append((code[start:i], start, c, i))
                if c == '{':
                    # skip the block
                    d, j = 1, i + 1
                    while j < n and d > 0:
                        if code[j] in '"\'':
                            q = code[j]
                            j += 1
                            while j < n and code[j] != q and code[j] != '\n':
                                j += 2 if code[j] == '\\' else 1
                        elif code[j] == '{':
                            d += 1
                        elif code[j] == '}':
                            d -= 1
                        j += 1
                    out[-1] = (code[start:i], start, '{', j)
                    i = j
                    # a struct/enum body is followed by its declarator up to ';'
                    k = i
                    while k < n and code[k] in ' \t\n':
                        k += 1
                    start = i
                    continue
                start = i + 1
            i += 1
        return out

    def comment_before(self, offset, after_offset):
        """The last comment block that ends between two code offsets (the doc block of a declaration)."""
        a, b = self.line_of(after_offset), self.line_of(offset)
        best = None
        for s, e, t in self.comments:
            if a <= s and e <= b:
                best = (s, e, t)
        return best


PROTO = re.compile(r'^\s*(?:extern\s+)?(?P<ret>(?:(?:const|volatile|unsigned|signed|struct|enum|static|inline)\s+)*[A-Za-z_]\w*(?:\s+(?:const|int|long|char|short))*[\s\*]+)'
                   r'(?P<name>[A-Za-z_]\w*)\s*\((?P<params>.*)\)(?P<attrs>.*)$', re.S)


def parse_proto(text):
    t = ' '.join(text.split())
    # declaration-block macros without a semicolon (__BEGIN_DECLS, __END_DECLS) before the statement
    t = re.sub(r'^(?:__[A-Z][A-Z_]*\s+)+', '', t)
    if not t or t.startswith(('typedef', 'return', 'if ', 'else', 'while', 'for ', 'switch', 'do ', 'case ', 'goto')):
        return None
    m = PROTO.match(t)
    if not m:
        return None
    ret = ' '.join(m.group('ret').split())
    if ret.split()[0] in ('return', 'typedef', 'else', 'case', 'goto', 'sizeof'):
        return None
    # the parameter list: balanced
    params_text = m.group('params')
    depth, cut = 0, None
    for i, ch in enumerate(params_text):
        if ch == '(':
            depth += 1
        elif ch == ')':
            if depth == 0:
                cut = i
                break
            depth -= 1
    if cut is not None:
        attrs = params_text[cut + 1:] + ')' + m.group('attrs')
        params_text = params_text[:cut]
    else:
        attrs = m.group('attrs')
    params, depth, cur = [], 0, ''
    for ch in params_text:
        if ch == ',' and depth == 0:
            params.append(cur.strip())
            cur = ''
            continue
        if ch in '([':
            depth += 1
        elif ch in ')]':
            depth -= 1
        cur += ch
    if cur.strip():
        params.append(cur.strip())
    if params == ['void']:
        params = []
    return {'ret': ret, 'name': m.group('name'), 'params': params, 'attrs': ' '.join(attrs.split())}


def param_name(p):
    if p == '...':
        return '...'
    m = re.search(r'\(\s*\*\s*(\w+)\s*\)', p)  # a function pointer
    if m:
        return m.group(1)
    m = re.search(r'(\w+)\s*(?:\[[^\]]*\]\s*)*$', p)
    return m.group(1) if m else ''


def param_type_words(p):
    words = re.findall(r'[A-Za-z_]\w*', p)
    n = param_name(p)
    if words and words[-1] == n and len(words) > 1:
        words = words[:-1]
    return words


def files_of(root, globs):
    out = []
    for g in globs.split(','):
        if not g:
            continue
        for p in sorted(glob.glob(os.path.join(root, g))):
            if os.path.isfile(p):
                out.append(os.path.relpath(p, root))
    seen, res = set(), []
    for r in out:
        if r not in seen:
            seen.add(r)
            res.append(r)
    return res


class Files:
    def __init__(self, root):
        self.root = root
        self.cache = {}

    def get(self, rel):
        if rel not in self.cache:
            self.cache[rel] = Src(self.root, rel)
        return self.cache[rel]


# ---------------------------------------------------------------- values
INT = re.compile(r'^\(*\s*(-?\s*(?:0[xX][0-9A-Fa-f]+|\d+))[uUlL]*\s*\)*$')


def typedef_widths(files, rels):
    """name -> set of (width, signed) over the typedefs of these files (a name typedef'd differently in #if branches keeps
    every resolution)."""
    raw = {}
    for r in rels:
        s = files.get(r)
        for m in re.finditer(r'\btypedef\s+([A-Za-z_][\w\s]*?)\s+([A-Za-z_]\w*)\s*;', s.code):
            raw.setdefault(m.group(2), set()).add(' '.join(m.group(1).split()))

    def res(name, seen):
        if name in WIDTHS:
            return {WIDTHS[name]}
        if name in seen or name not in raw:
            return set()
        out = set()
        for base in raw[name]:
            b = base
            if b in WIDTHS:
                out.add(WIDTHS[b])
            else:
                out |= res(b, seen | {name})
        return out

    return {n: res(n, frozenset()) for n in raw}


def int_value(text, widths, names):
    t = text.strip()
    t = re.sub(r'/\*.*?\*/', '', t).strip()
    cast = None
    m = re.match(r'^\(\s*\(\s*([A-Za-z_]\w*)\s*\)\s*(.*)\)$', t)
    if m:
        cast, t = m.group(1), m.group(2).strip()
    m = re.match(r'^[A-Za-z_]\w*\s*\(\s*(.*)\s*\)$', t)  # a value macro such as CFE_STATUS_C(x)
    if m and not INT.match(t):
        t = m.group(1)
    m = INT.match(t)
    if not m:
        m2 = re.match(r'^\(?\s*([A-Za-z_]\w*)\s*\)?$', t)
        if m2 and m2.group(1) in names:
            return names[m2.group(1)]
        return None
    # the value of the literal as written: a cast is not applied (the value the facts' RET_CONSTS give for it)
    return int(m.group(1).replace(' ', ''), 0)


def group_in(files, spec, widths):
    rel, _, rng = spec.rpartition(':')
    a, b = (int(x) for x in rng.split('-'))
    s = files.get(rel)
    out, names = [], {}
    for ln in range(a, b + 1):
        l = s.lines[ln - 1] if ln - 1 < len(s.lines) else ''
        m = re.match(r'^\s*#\s*define\s+([A-Za-z_]\w*)\s+(.+?)\s*$', l)
        if not m:
            m = re.match(r'^\s*([A-Za-z_]\w*)\s*=\s*([^,]+?)\s*,?\s*$', l)
        if not m:
            continue
        v = int_value(m.group(2), widths, names)
        if v is None:
            continue
        names[m.group(1)] = v
        out.append((m.group(1), v, rel, ln))
    return out


# ---------------------------------------------------------------- doc blocks
RET_LINE = re.compile(r'[@\\](?:retval|retcode|returns?)\b\s*#?([A-Za-z_]\w*)?')
PARAM_LINE = re.compile(r'[@\\]param(?:\s*\[[^\]]*\])?\s+\**\s*([A-Za-z_]\w*)')
STOP_LINE = re.compile(r'[@\\](?:param|retval|retcode|returns?|endreturns|sa|par|note|brief|details|see)\b')


def doc_items(comment, gs, ge, gr):
    """(returns [(name, line)], params {param name: [(kind, name, line)]}) of a doc block."""
    if comment is None:
        return [], {}
    start, _, text = comment
    rets, params, cur = [], {}, None
    for k, l in enumerate(text.split('\n')):
        ln = start + k
        pm = PARAM_LINE.search(l)
        if pm:
            cur = pm.group(1)
            params.setdefault(cur, [])
        elif STOP_LINE.search(l):
            cur = None
        for m in RET_LINE.finditer(l):
            if m.group(1) and m.group(1) in gs:
                rets.append((m.group(1), ln))
        if cur is not None:
            for m in re.finditer(r'#([A-Za-z_]\w*)', l):
                n = m.group(1)
                if n in ge:
                    params[cur].append(('event-type', n, ln))
                elif n in gr:
                    params[cur].append(('run-status', n, ln))
    return rets, params


# ---------------------------------------------------------------- definitions
def definitions(files, rels):
    """name -> [(rel, line, body start offset, body end offset)] of the function definitions in these files."""
    out = {}
    for r in rels:
        s = files.get(r)
        prev = 0
        for text, start, end_ch, end in s.statements():
            if end_ch == '{':
                p = parse_proto(text)
                if p and not text.strip().startswith(('struct', 'enum', 'union', 'typedef')) and '=' not in text.split('(')[0]:
                    off = start + text.find(p['name'])
                    # the brace offset: find the '{' right after the statement text
                    bo = s.code_np.find('{', start + len(text))
                    out.setdefault(p['name'], []).append((r, s.line_of(off), bo, end, p))
            prev = end
    return out


def body_lines(s, bo, end):
    a, b = s.line_of(bo), s.line_of(end)
    return [(ln, s.lines[ln - 1]) for ln in range(a, b + 1) if ln - 1 < len(s.lines)]


# ---------------------------------------------------------------- the framework scan
def scan_component(comp, p0cmp):
    root = comp['root']
    files = Files(root)
    headers = files_of(root, comp.get('headers', ''))
    sources = files_of(root, comp.get('sources', ''))
    widths = typedef_widths(files, headers)
    pinned = {}

    def pin(rel):
        pinned[rel] = files.get(rel).sha256
        return rel

    groups = {}
    for kind in ('status', 'event-type', 'run-status'):
        if comp.get(kind):
            g = group_in(files, comp[kind], widths)
            groups[kind] = g
            pin(comp[kind].rpartition(':')[0])
    gs = {n for n, _, _, _ in groups.get('status', [])}
    ge = {n for n, _, _, _ in groups.get('event-type', [])}
    gr = {n for n, _, _, _ in groups.get('run-status', [])}
    gval = {n: v for k in groups for n, v, _, _ in groups[k]}
    types = {'status': set(filter(None, comp.get('status-types', '').split(','))),
             'event-type': set(filter(None, comp.get('event-type-types', '').split(','))),
             'run-status': set(filter(None, comp.get('run-status-types', '').split(',')))}
    status_space = []
    if comp.get('success'):
        for n, v, r, ln in groups.get('status', []):
            if n == comp['success']:
                status_space.append(('success', n, v, r, ln))
    if comp.get('severity'):
        sev = comp['severity'].split(',')
        for r in headers:
            s = files.get(r)
            for k, l in enumerate(s.lines, 1):
                m = re.match(r'^\s*#\s*define\s+([A-Za-z_]\w*)\s+(.+?)\s*$', l)
                if m and m.group(1) in sev:
                    v = int_value(m.group(2), widths, {})
                    if v is not None:
                        status_space.append(('severity', m.group(1), v, pin(r), k))
    defs = definitions(files, sources)
    entries = []
    for r in headers:
        s = files.get(r)
        prev_end = 0
        for text, start, end_ch, end in s.statements():
            if end_ch != ';':
                prev_end = end
                continue
            p = parse_proto(text)
            if not p or p['name'] in ('defined',):
                prev_end = end
                continue
            first = start + (len(text) - len(text.lstrip()))
            off = start + text.find(p['name'] + ' ') if (p['name'] + ' ') in text else start + text.find(p['name'])
            line = s.line_of(off)
            doc = s.comment_before(first, prev_end)
            prev_end = end
            rets, pdoc = doc_items(doc, gs, ge, gr)
            reasons = []
            ret_words = re.findall(r'[A-Za-z_]\w*', p['ret'])
            ret_kind = None
            for kind in ('status', 'event-type', 'run-status'):
                if types[kind] & set(ret_words):
                    reasons.append('E1')
                    ret_kind = (kind, 'typed')
            args = []
            for i, pr in enumerate(p['params']):
                words = set(param_type_words(pr))
                for kind in ('status', 'event-type', 'run-status'):
                    if types[kind] & words:
                        args.append((i, kind, 'typed', r, line))
                        if 'E1' not in reasons:
                            reasons.append('E1')
                pn = param_name(pr)
                for kind, cname, ln in pdoc.get(pn, []):
                    if not any(a[0] == i and a[1] == kind and a[2] == 'doc' for a in args):
                        args.append((i, kind, 'doc', r, ln))
                    if 'E2' not in reasons:
                        reasons.append('E2')
            if rets:
                if 'E2' not in reasons:
                    reasons.append('E2')
                if ret_kind is None:
                    ret_kind = ('status', 'doc')
                elif 'doc' not in ret_kind[1]:
                    ret_kind = (ret_kind[0], ret_kind[1] + '+doc')
            body = []
            dl = defs.get(p['name'], [])
            dsel = dl[0] if len(dl) == 1 else None
            if dsel is not None:
                ds = files.get(dsel[0])
                for ln, l in body_lines(ds, dsel[2], dsel[3]):
                    for m in re.finditer(r'\breturn\s*\(?\s*([A-Za-z_]\w*)\s*\)?\s*;', l):
                        if m.group(1) in gs:
                            body.append((m.group(1), ln))
                if body:
                    if 'E3' not in reasons:
                        reasons.append('E3')
                    if ret_kind is None:
                        ret_kind = ('status', 'body')
                    elif 'body' not in ret_kind[1]:
                        ret_kind = (ret_kind[0], ret_kind[1] + '+body')
            if not reasons:
                continue
            pin(r)
            d = '-'
            if dsel is not None:
                d = '%s:%d' % (pin(dsel[0]), dsel[1])
            entries.append({'name': p['name'], 'decl': (r, line), 'def': d, 'ndefs': len(dl), 'reasons': reasons,
                            'ret': ret_kind, 'rets': rets, 'args': sorted(set(args)), 'body': body})
    # one entry per name: a name declared twice keeps its first declaration (both are reported)
    seen, dup, uniq = {}, [], []
    for e in entries:
        if e['name'] in seen:
            dup.append(e)
            continue
        seen[e['name']] = e
        uniq.append(e)
    ns = namespaces(comp, files, headers, sources, defs, gs, p0cmp, pin)
    declared = set()
    for r in headers:
        for text, start, end_ch, end in files.get(r).statements():
            if end_ch == ';':
                p = parse_proto(text)
                if p:
                    declared.add(p['name'])
    return {'declared': len(declared), 'groups': groups, 'gval': gval, 'status_space': status_space, 'entries': uniq, 'dup': dup, 'pinned': pinned,
            'headers': headers, 'sources': sources, 'namespaces': ns}


def namespaces(comp, files, headers, sources, defs, gs, p0cmp, pin):
    out = []
    public = set()
    for r in headers:
        s = files.get(r)
        for text, start, end_ch, end in s.statements():
            if end_ch == ';':
                p = parse_proto(text)
                if p:
                    public.add(p['name'])
    cmpre = r'\b(%s)\s*\(' % '|'.join(re.escape(n) for n in p0cmp)
    if comp.get('objid'):
        lookup, create, delete, init, kmacro, nfield = comp['objid'].split(',')
        # the kinds: a kind macro assignment per source file
        kinds = {}
        for r in sources:
            s = files.get(r)
            for k, l in enumerate(s.lines, 1):
                m = re.search(r'\b%s\s*=\s*([A-Za-z_]\w*)' % re.escape(kmacro), l) or re.search(r'#\s*define\s+%s\s+([A-Za-z_]\w*)' % re.escape(kmacro), l)
                if m:
                    kinds.setdefault(r, (m.group(1), k))
        kval = {}
        for r in headers:
            s = files.get(r)
            for k, l in enumerate(s.lines, 1):
                m = re.match(r'^\s*#\s*define\s+([A-Za-z_]\w*)\s+(0[xX][0-9A-Fa-f]+|\d+)\b', l)
                if m:
                    kval[m.group(1)] = (int(m.group(2), 0), r, k)
        # the comparison function: compares its parameter with the name field through a P0 byte-string comparison
        comparisons = []
        for name, dl in defs.items():
            for (r, line, bo, end, p) in dl:
                s = files.get(r)
                pnames = [param_name(x) for x in p['params']]
                for ln, l in body_lines(s, bo, end):
                    m = re.search(cmpre, l)
                    if m and re.search(r'(->|\.)\s*%s\b' % re.escape(nfield), l) and any(re.search(r'\b%s\b' % re.escape(q), l) for q in pnames if q):
                        comparisons.append((name, r, line, m.group(1), ln))
        stores = []  # other stores into the name field
        for r in sources:
            s = files.get(r)
            for k, l in enumerate(s.lines, 1):
                if re.search(r'(->|\.)\s*%s\s*=[^=]' % re.escape(nfield), l):
                    stores.append((r, k))
        # the status constants in the lookup layer's chain (its body and the functions of its file it calls, depth 3)
        chain = []
        if lookup in defs and len(defs[lookup]) == 1:
            todo, seen = [(lookup, 0)], set()
            while todo:
                fn, depth = todo.pop(0)
                if fn in seen or fn not in defs or len(defs[fn]) != 1:
                    continue
                seen.add(fn)
                r, line, bo, end, p = defs[fn][0]
                s = files.get(r)
                for ln, l in body_lines(s, bo, end):
                    for m in re.finditer(r'\b([A-Za-z_]\w*)\b', l):
                        if m.group(1) in gs:
                            chain.append((fn, r, line, m.group(1), ln))
                        elif depth < 3 and m.group(1) in defs and m.group(1) != fn and re.search(r'\b%s\s*\(' % re.escape(m.group(1)), l):
                            if defs[m.group(1)][0][0] == r:
                                todo.append((m.group(1), depth + 1))
        lines_by_kind = []
        for r, (kconst, kline) in sorted(kinds.items()):
            s = files.get(r)
            ent = {'kind': kconst, 'kind_at': (r, kline), 'value': kval.get(kconst), 'lookup': [], 'create': [], 'delete': [], 'name_field': [], 'stores': []}
            for name, dl in sorted(defs.items()):
                for (dr, line, bo, end, p) in dl:
                    if dr != r:
                        continue
                    for ln, l in body_lines(s, bo, end):
                        if re.search(r'\b%s\s*\(\s*%s\b' % (re.escape(lookup), re.escape(kmacro)), l):
                            ent['lookup'].append((name, r, line, ln, name in public))
                        if re.search(r'\b%s\s*\(\s*%s\b' % (re.escape(create), re.escape(kmacro)), l):
                            ent['create'].append((name, r, line, ln, name in public))
                        if re.search(r'\b%s\s*\(' % re.escape(delete), l):
                            ent['delete'].append((name, r, line, ln, name in public))
                        m = re.search(r'\b%s\s*\(\s*\w+\s*,\s*(\w+)\s*,\s*(\w+)\s*,' % re.escape(init), l)
                        if m:
                            ent['name_field'].append((name, m.group(1), m.group(2), r, ln, record_field(files, sources + headers_all(comp, files), s, bo, end, m.group(1), m.group(2))))
            ent['stores'] = [(sr, sk) for (sr, sk) in stores if sr == r]
            lines_by_kind.append(ent)
        for c in comparisons:
            pin(c[1])
        for e in lines_by_kind:
            pin(e['kind_at'][0])
            if e['value']:
                pin(e['value'][1])
            for nf in e['name_field']:
                if nf[5]:
                    pin(nf[5][1])
        for c in chain:
            pin(c[1])
        out.append({'rule': 'N', 'comparisons': comparisons, 'chain': chain, 'kinds': lines_by_kind, 'anchors': comp['objid']})
        return out
    # rule T: the table generation
    sites = []
    for name, dl in sorted(defs.items()):
        if len(dl) != 1:
            continue
        r, line, bo, end, p = dl[0]
        s = files.get(r)
        pnames = [param_name(x) for x in p['params'] if param_name(x)]
        for ln, l in body_lines(s, bo, end):
            for m in re.finditer(cmpre, l):
                args = call_args(l, m.end() - 1)
                if args is None or len(args) < 2:
                    continue
                a0, a1 = strip_cast(args[0]), strip_cast(args[1])
                for par, other in ((a0, a1), (a1, a0)):
                    if par in pnames and re.search(r'\[[^\]]+\]', other) and re.fullmatch(r'[A-Za-z_][\w\.\[\]\->\s]*', other):
                        sites.append({'fn': name, 'public': name in public, 'def': (r, line), 'cmp': m.group(1), 'at': (r, ln),
                                      'path': norm_path(other), 'raw': other.strip(), 'bound': args[2].strip() if len(args) > 2 else ''})
    by_path = {}
    for st in sites:
        by_path.setdefault(st['path'], []).append(st)
    for path, sts in sorted(by_path.items()):
        ent = {'path': path, 'lookup': [], 'compare_only': [], 'copy': [], 'clear': [], 'sites': sts, 'extent': None}
        pre = re.escape(path).replace(r'\[\]', r'\[[^\]]*\]')
        for name, dl in sorted(defs.items()):
            for (r, line, bo, end, p) in dl:
                s = files.get(r)
                pnames = [param_name(x) for x in p['params'] if param_name(x)]
                for ln, l in body_lines(s, bo, end):
                    m = re.search(r'\b(strcpy|strncpy|memcpy|snprintf)\s*\(\s*(?:\(\s*char\s*\*\s*\)\s*)?' + pre + r'\s*,\s*(.*)', l)
                    if m:
                        src = m.group(2)
                        if re.match(r'^\s*""', src):
                            ent['clear'].append((name, r, line, ln))
                        elif any(re.match(r'^\s*(?:\(\s*\w+\s*\**\s*\)\s*)?%s\b' % re.escape(q), src) for q in pnames):
                            ent['copy'].append((name, r, line, ln))
                    if re.search(pre + r'\s*\[\s*0\s*\]\s*=\s*(?:\'\\0\'|0)\s*;', l):
                        ent['clear'].append((name, r, line, ln))
        copiers = {c[0] for c in ent['copy']}
        for st in sts:
            if st['public']:
                (ent['compare_only'] if st['fn'] in copiers else ent['lookup']).append(st)
        ent['extent'] = path_extent(files, sources + headers_all(comp, files), sts[0]['raw'])
        if ent['lookup'] or ent['compare_only']:
            for st in sts:
                pin(st['def'][0])
            for c in ent['copy'] + ent['clear']:
                pin(c[1])
            if ent['extent']:
                pin(ent['extent'][1])
            out.append(dict(ent, rule='T'))
    return out


def headers_all(comp, files):
    # every header under the root's component directories the sources include from (for the declarations of records)
    root = comp['root']
    out = []
    for g in comp.get('headers', '').split(',') + comp.get('sources', '').split(','):
        d = os.path.dirname(os.path.join(root, g))
        top = d
        for _ in range(3):
            top = os.path.dirname(top)
        for p in sorted(glob.glob(os.path.join(top, '**', '*.h'), recursive=True)):
            rel = os.path.relpath(p, root)
            if any(x in rel for x in ('unit-test', 'ut-stubs', 'ut-coverage', 'ut_', '/ut/', 'unit_test', 'tests/')):
                continue
            out.append(rel)
    seen, res = set(), []
    for r in out:
        if r not in seen:
            seen.add(r)
            res.append(r)
    return res


def call_args(l, paren):
    depth, cur, args = 0, '', []
    for ch in l[paren:]:
        if ch == '(':
            depth += 1
            if depth == 1:
                continue
        elif ch == ')':
            depth -= 1
            if depth == 0:
                args.append(cur)
                return args
        if ch == ',' and depth == 1:
            args.append(cur)
            cur = ''
            continue
        cur += ch
    return None


def strip_cast(a):
    a = a.strip()
    while True:
        m = re.match(r'^\(\s*(?:const\s+)?(?:unsigned\s+)?[A-Za-z_]\w*\s*\**\s*\)\s*(.*)$', a)
        if not m:
            return a
        a = m.group(1).strip()


def norm_path(p):
    return re.sub(r'\s+', '', re.sub(r'\[[^\]]*\]', '[]', p))


def find_struct_field(files, rels, typename, field):
    """(element type, file, line, extent text) of a member `field` of the record type `typename`."""
    for r in rels:
        try:
            s = files.get(r)
        except OSError:
            continue
        code = s.code
        for m in re.finditer(r'\btypedef\s+struct\s*\w*\s*\{|\bstruct\s+(\w+)\s*\{', code):
            # the body and the name
            bo = code.find('{', m.start())
            d, j = 1, bo + 1
            while j < len(code) and d > 0:
                if code[j] == '{':
                    d += 1
                elif code[j] == '}':
                    d -= 1
                j += 1
            body = code[bo + 1:j - 1]
            tail = code[j:j + 200]
            names = set()
            if m.group(1):
                names.add(m.group(1))
            mt = re.match(r'\s*(\w+)\s*;', tail)
            if mt and code[m.start():m.start() + 7] == 'typedef':
                names.add(mt.group(1))
            if typename not in names:
                continue
            for fm in re.finditer(r'([A-Za-z_][\w\s\*]*?)\s+\**\s*%s\s*((?:\[[^\]]*\]\s*)*);' % re.escape(field), body):
                ty = ' '.join(fm.group(1).split())
                ext = fm.group(2).strip()
                return ty, r, s.line_of(bo + 1 + fm.start(2)), ext
    return None


def find_decl_type(files, rels, ident):
    for r in rels:
        try:
            s = files.get(r)
        except OSError:
            continue
        for m in re.finditer(r'(?m)^\s*(?:extern\s+|static\s+)?([A-Za-z_]\w*)\s+%s\s*(?:\[[^\]]*\]\s*)*[;=]' % re.escape(ident), s.code):
            if m.group(1) not in ('return', 'typedef'):
                return m.group(1)
    return None


def path_extent(files, rels, raw):
    """The declared extent of the last field of a table path X[i].a.b (types through the declarations), or None."""
    steps = re.findall(r'[A-Za-z_]\w*', re.sub(r'\[[^\]]*\]', '', raw))
    if len(steps) < 2:
        return None
    ty = find_decl_type(files, rels, steps[0])
    if ty is None:
        return None
    hit = None
    for f in steps[1:]:
        hit = find_struct_field(files, rels, ty, f)
        if hit is None:
            return None
        ty = hit[0].split()[-1]
    ext = hit[3]
    if not ext.startswith('[') or not byte_type(files, rels, hit[0]):
        return None
    return (ext.strip('[] '), hit[1], hit[2], hit[0])


BYTE_TYPES = {'char', 'signed char', 'unsigned char', 'int8_t', 'uint8_t'}


def byte_type(files, rels, ty):
    """Is the element type one byte wide: a character type, or a typedef of one in the files of the component."""
    ty = ' '.join(w for w in ty.split() if w not in ('const', 'volatile'))
    if ty in BYTE_TYPES:
        return True
    root = files.root
    seen = set()
    while ty not in seen:
        seen.add(ty)
        base = None
        for r in rels + typedef_files(files):
            s = files.get(r)
            m = re.search(r'\btypedef\s+([A-Za-z_][\w\s]*?)\s+%s\s*;' % re.escape(ty), s.code)
            if m:
                base = ' '.join(m.group(1).split())
                break
        if base is None:
            return False
        if base in BYTE_TYPES:
            return True
        ty = base
    return False


def typedef_files(files):
    if not hasattr(files, '_tdf'):
        files._tdf = sorted(os.path.relpath(p, files.root) for p in glob.glob(os.path.join(files.root, '**', 'inc', '*.h'), recursive=True)
                            if not any(x in p for x in ('unit-test', 'ut-stubs', 'ut-coverage', '/ut/')))
    return files._tdf


def record_field(files, rels, s, bo, end, rec, field):
    """The character-array declaration of `field` in the record type of the local `rec` of a function body."""
    text = s.code[bo:end]
    m = re.search(r'\b([A-Za-z_]\w*)\s*\*\s*%s\s*[;=]' % re.escape(rec), text)
    if not m:
        return None
    hit = find_struct_field(files, rels, m.group(1), field)
    if hit is None or not hit[3].startswith('[') or not byte_type(files, rels, hit[0]):
        return None
    return (hit[3].strip('[] '), hit[1], hit[2], hit[0])


# ---------------------------------------------------------------- the p0 scan
P0_ATTRS = [('pure', r'__attribute_pure__|__attribute__\s*\(\(\s*__pure__'), ('const', r'__attribute_const__|__attribute__\s*\(\(\s*__const__'),
            ('noreturn', r'__attribute_noreturn__|__attribute__\s*\(\(\s*__noreturn__|_Noreturn'), ('warn-unused-result', r'__wur\b|__attribute_warn_unused_result__'),
            ('nothrow', r'__THROW\b|__THROWNL\b')]


def scan_p0(comp, names):
    root = comp['root']
    files = Files(root)
    headers = files_of(root, comp.get('headers', ''))
    found = {}
    for r in headers:
        s = files.get(r)
        for text, start, end_ch, end in s.statements():
            if end_ch != ';':
                continue
            p = parse_proto(text)
            if not p or p['name'] not in dict(names) or p['name'] in found:
                continue
            off = start + text.find(p['name'])
            attrs = []
            for a, rx in P0_ATTRS:
                if re.search(rx, text):
                    attrs.append(a)
            nn = re.search(r'__nonnull\s*\(\(\s*([\d\s,]+)\)\)', text)
            if nn:
                attrs.append('nonnull(%s)' % ','.join(x.strip() for x in nn.group(1).split(',') if x.strip()))
            found[p['name']] = (r, s.line_of(off), attrs, s.sha256)
    src = comp.get('p0-source', '')
    return {'found': found, 'missing': [n for n, _ in names if n not in found], 'source': (src, sha(src)) if src and os.path.isfile(src) else None}


# ---------------------------------------------------------------- output
def head(label, comp, kind):
    return ['# %s -- semantics data of %s, layer %s, generated by research env/tools/semantics_scan.py (%s); do not edit:'
            % ('%s_%s.txt' % (kind, label), label, comp['layer'], RULE_REVISION),
            '# the file is what the scan of the version-pinned files below gives; `semantics_scan.py check` regenerates it and',
            '# compares.  Every file:line is relative to the component root and pinned by the sha256 of its `file` line.',
            FORMAT, 'layer %s' % comp['layer'], 'component %s version="%s"' % (label, comp['version'])]


def fw_lines(comp, sc):
    label = comp['label']
    L = head(label, comp, 'framework_semantics')
    L.append('rule %s' % RULE_REVISION)
    for rel in sorted(sc['pinned']):
        L.append('file %s sha256=%s' % (rel, sc['pinned'][rel]))
    for kind in ('status', 'event-type', 'run-status'):
        for n, v, r, ln in sc['groups'].get(kind, []):
            L.append('group %s %s %d %s:%d' % (kind, n, v, r, ln))
    for what, n, v, r, ln in sc['status_space']:
        L.append('status-space %s %s %d %s:%d' % (what, n, v, r, ln))
    for e in sc['entries']:
        rk = e['ret']
        L.append('api %s decl=%s:%d def=%s return=%s by=%s rule=%s' % (e['name'], e['decl'][0], e['decl'][1], e['def'], rk[0] if rk else 'none', rk[1] if rk else '-',
                                                                       '+'.join(e['reasons'])))
        for n, ln in e['rets']:
            L.append('ret %s %s %d %s:%d' % (e['name'], n, sc['gval'][n], e['decl'][0], ln))
        for i, kind, by, r, ln in e['args']:
            L.append('arg %s %d %s by=%s %s:%d' % (e['name'], i, kind, by, r, ln))
        for n, ln in e['body']:
            L.append('body-ret %s %s %d %s:%d' % (e['name'], n, sc['gval'][n], e['def'].rpartition(':')[0], ln))
    L.append('end entries=%d declared=%d second-declarations=%d' % (len(sc['entries']), sc['declared'], len(sc['dup'])))
    return L


def ns_lines(comp, sc):
    label = comp['label']
    L = head(label, comp, 'framework_namespaces')
    L.append('rule %s' % RULE_REVISION)
    pins = set()
    body = []
    n = 0
    for ns in sc['namespaces']:
        if ns['rule'] == 'N':
            body.append('generation object-id anchors=%s' % ns['anchors'])
            for fn, r, line, cmp, ln in ns['comparisons']:
                body.append('compare %s def=%s:%d call=%s at=%s:%d' % (fn, r, line, cmp, r, ln))
                pins.add(r)
            for fn, r, line, c, ln in ns['chain']:
                body.append('chain-status %s def=%s:%d %s at=%s:%d' % (fn, r, line, c, r, ln))
                pins.add(r)
            for e in ns['kinds']:
                n += 1
                v = e['value']
                body.append('kind %s value=%s at=%s source=%s:%d' % (e['kind'], v[0] if v else '-', '%s:%d' % (v[1], v[2]) if v else '-', e['kind_at'][0], e['kind_at'][1]))
                pins.add(e['kind_at'][0])
                if v:
                    pins.add(v[1])
                for role in ('lookup', 'create', 'delete'):
                    for fn, r, line, ln, pub in e[role]:
                        body.append('  %s %s def=%s:%d at=%s:%d public=%s' % (role, fn, r, line, r, ln, 'yes' if pub else 'no'))
                for fn, rec, field, r, ln, ext in e['name_field']:
                    if ext:
                        body.append('  stored-name %s at=%s:%d extent=%s element=%s decl=%s:%d' % (fn, r, ln, ext[0], ext[3].replace(' ', '-'), ext[1], ext[2]))
                        pins.add(ext[1])
                    else:
                        body.append('  stored-name %s at=%s:%d extent=- (the record field declaration is not found)' % (fn, r, ln))
                for sr, sk in e['stores']:
                    body.append('  other-store at=%s:%d' % (sr, sk))
        else:
            n += 1
            body.append('table %s' % ns['path'])
            for st in ns['sites']:
                body.append('  compare %s def=%s:%d call=%s at=%s:%d bound=%s public=%s' % (st['fn'], st['def'][0], st['def'][1], st['cmp'], st['at'][0], st['at'][1],
                                                                                           re.sub(r'\s+', '', st['bound']) or '-', 'yes' if st['public'] else 'no'))
                pins.add(st['def'][0])
            for st in ns['lookup']:
                body.append('  lookup %s def=%s:%d' % (st['fn'], st['def'][0], st['def'][1]))
            for st in ns['compare_only']:
                body.append('  create-check %s def=%s:%d' % (st['fn'], st['def'][0], st['def'][1]))
            for fn, r, line, ln in ns['copy']:
                body.append('  copy %s def=%s:%d at=%s:%d' % (fn, r, line, r, ln))
                pins.add(r)
            for fn, r, line, ln in ns['clear']:
                body.append('  clear %s def=%s:%d at=%s:%d' % (fn, r, line, r, ln))
                pins.add(r)
            if ns['extent']:
                body.append('  extent %s element=%s decl=%s:%d' % (ns['extent'][0], ns['extent'][3].replace(' ', '-'), ns['extent'][1], ns['extent'][2]))
                pins.add(ns['extent'][1])
            else:
                body.append('  extent - (no one-byte array declaration of the last field is found through the declared types)')
    files = Files(comp['root'])
    for rel in sorted(pins):
        L.append('file %s sha256=%s' % (rel, files.get(rel).sha256))
    L += body
    L.append('end namespaces=%d' % n)
    return L


def p0_lines(comp, sc, names):
    label = comp['label']
    L = head(label, comp, 'p0_declarations')
    L.append('rule %s' % RULE_REVISION)
    pins = {}
    for n, (r, ln, attrs, s) in sorted(sc['found'].items()):
        pins[r] = s
    for rel in sorted(pins):
        L.append('file %s sha256=%s' % (rel, pins[rel]))
    if sc['source']:
        # the local copy by its file name and sha256 (its directory is a path of this machine, not data)
        L.append('source %s sha256=%s' % (os.path.basename(sc['source'][0]), sc['source'][1]))
    for n, t in names:
        if n in sc['found']:
            r, ln, attrs, _ = sc['found'][n]
            L.append('api %s decl=%s:%d table=%s attrs=%s' % (n, r, ln, t, ','.join(attrs) if attrs else '-'))
        else:
            L.append('missing %s table=%s (no prototype of the name in the listed headers)' % (n, t))
    L.append('end entries=%d missing=%d' % (len(sc['found']), len(sc['missing'])))
    return L


def generate_all(trees, p0cpp):
    comps = read_trees(trees)
    names = p0_names(p0cpp)
    p0cmp = [n for n, t in names if t == 'compare']
    # only the byte-string comparisons of the compare table name a key comparison
    text = open(p0cpp).read()
    p0cmp = [n for n in p0cmp if re.search(r'\{"%s", "byte-string comparison"' % re.escape(n), text)]
    out = {}
    for c in comps:
        if c['layer'] == 'p0':
            sc = scan_p0(c, names)
            out['p0_declarations_%s.txt' % c['label']] = p0_lines(c, sc, names)
            continue
        sc = scan_component(c, p0cmp)
        out['framework_semantics_%s.txt' % c['label']] = fw_lines(c, sc)
        if sc['namespaces']:
            out['framework_namespaces_%s.txt' % c['label']] = ns_lines(c, sc)
    return comps, out


def main(argv):
    if len(argv) == 4 and argv[1] == 'scan':
        _, out = generate_all(argv[2], argv[3])
        for name in sorted(out):
            print('== %s' % name)
            for l in out[name]:
                print(l)
        return 0
    if len(argv) == 5 and argv[1] == 'generate':
        _, out = generate_all(argv[2], argv[3])
        os.makedirs(argv[4], exist_ok=True)
        for name in sorted(out):
            p = os.path.join(argv[4], name)
            if os.path.exists(p):
                sys.stderr.write('refusing: %s exists\n' % p)
                return 2
        for name in sorted(out):
            with open(os.path.join(argv[4], name), 'w') as f:
                f.write('\n'.join(out[name]) + '\n')
            print('wrote %s (%d lines)' % (name, len(out[name])))
        return 0
    if len(argv) == 5 and argv[1] == 'check':
        comps, out = generate_all(argv[2], argv[3])
        bad = 0
        present = sorted(n for n in os.listdir(argv[4]) if re.match(r'^(framework_semantics|framework_namespaces|p0_declarations)_.*\.txt$', n))
        for name in sorted(set(out) | set(present)):
            p = os.path.join(argv[4], name)
            if name not in out:
                print('FAIL %s: a data file the scan does not generate' % name)
                bad += 1
                continue
            if not os.path.isfile(p):
                print('FAIL %s: generated by the scan, not in the data directory' % name)
                bad += 1
                continue
            have = open(p).read().split('\n')
            if have and have[-1] == '':
                have = have[:-1]
            if have != out[name]:
                diff = [i + 1 for i, (a, b) in enumerate(zip(have, out[name])) if a != b]
                print('FAIL %s: differs from the scan (%d lines on file, %d generated; first differing line %s)'
                      % (name, len(have), len(out[name]), diff[0] if diff else min(len(have), len(out[name])) + 1))
                bad += 1
                continue
            # every pinned file holds its sha256, every cited line exists
            comp = next(c for c in comps if name.endswith('_%s.txt' % c['label']))
            root = comp['root']
            pins = {}
            for l in have:
                m = re.match(r'^file (\S+) sha256=([0-9a-f]{64})$', l)
                if m:
                    pins[m.group(1)] = m.group(2)
            nref, nbad = 0, 0
            for rel, sh in pins.items():
                if sha(os.path.join(root, rel)) != sh:
                    print('FAIL %s: %s sha256 differs' % (name, rel))
                    nbad += 1
            for l in have:
                m = re.match(r'^source (\S+) sha256=([0-9a-f]{64})$', l)
                if m:
                    src = comp.get('p0-source', '')
                    if not src or os.path.basename(src) != m.group(1) or sha(src) != m.group(2):
                        print('FAIL %s: the source line does not name the p0-source copy with its sha256' % name)
                        nbad += 1
            for l in have:
                for m in re.finditer(r'(?:^|[ =])([\w./+-]+\.[ch]):(\d+)\b', l):
                    rel, ln = m.group(1), int(m.group(2))
                    nref += 1
                    if rel not in pins:
                        print('FAIL %s: %s is cited but not pinned (%s)' % (name, rel, l[:80]))
                        nbad += 1
                        continue
                    if ln > len(open(os.path.join(root, rel), errors='replace').read().split('\n')):
                        print('FAIL %s: %s:%d is past the end of the file' % (name, rel, ln))
                        nbad += 1
            bad += nbad
            n_entries = sum(1 for l in have if l.startswith(('api ', 'kind ', 'table ')))
            print('%s %s: equal to the scan; %d lines, %d entries, %d pinned files, %d file:line references%s'
                  % ('ok  ' if nbad == 0 else 'FAIL', name, len(have), n_entries, len(pins), nref, '' if nbad == 0 else ', %d failures' % nbad))
        print('%s: %d data files, %d failures' % ('PASS' if bad == 0 else 'FAIL', len(out), bad))
        return 0 if bad == 0 else 1
    sys.stderr.write('usage: semantics_scan.py scan|generate|check <trees.tsv> <p0.cpp> [<dir>]\n')
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
