#!/usr/bin/env python3
"""stop_eval.py -- evaluate the stop event of a natural boot afterwards, from its record.

Run with `python3 -I`.  Reads only.

    stop_eval.py --manifest PLAN.kzs --sidecar PLAN.side --startup-script SCR --dump-text DUMP.txt [-o OUT]

The stop event (by rule): every application of the instance startup script reaches its first run-loop call or its
exit.  The harness cannot stop on an event (fixed waits; the record pool is written at process end), so the event is
evaluated here, afterwards, from the record:
  - the run-loop and exit APIs are the sidecar's "stop-event <role> <API> <source:line> ..." lines (framework-layer
    data), their sites the manifest's sites whose label names the stop-event rule and whose op is entry.<API>;
  - the applications are the CFE_APP lines of the startup script (type, file, entry, name; the file ends at a line
    that starts with '!'); a CFE_LIB line is a library: no stop event by the rule, listed as such;
  - the actor of an application is the actor that recorded the application's root entry (the manifest's site whose
    label names the actor-root rule and whose op is entry.<entry function>); no root site in the plan, or no such
    record, leaves the application unresolved;
  - its stop event is the first event of that actor at a stop-event site after its root entry (aseq order).
Times are the dump's own clock (ns; the RUN line's t_init and t_stop bound the recorded window); they are printed as
milliseconds after t_init.  The script states what was recorded; it does not judge the run normal or failed.
Exit status: 0 = every application reached its stop event inside the window, 3 = at least one did not or is
unresolved, 2 = usage or read error.
"""
import argparse
import re
import sys


def read(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        return f.read().split('\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--sidecar', required=True)
    ap.add_argument('--startup-script', required=True)
    ap.add_argument('--dump-text', required=True)
    ap.add_argument('-o', dest='out')
    a = ap.parse_args()
    try:
        man, side, scr, dump = read(a.manifest), read(a.sidecar), read(a.startup_script), read(a.dump_text)
    except OSError as e:
        sys.stderr.write('stop_eval.py: %s\n' % e)
        return 2

    apis = {}
    for line in side:
        m = re.match(r'stop-event (\S+) (\S+) (\S+)', line)
        if m:
            apis[m.group(2)] = (m.group(1), m.group(3))
    stop_sites, root_sites = {}, {}
    for line in man:
        m = re.match(r'site (\d+) label=\S+\.stop-event op=entry\.(\S+) ', line)
        if m and m.group(2) in apis:
            stop_sites[int(m.group(1))] = m.group(2)
        m = re.match(r'site (\d+) label=\S+\.actor-root op=entry\.(\S+) ', line)
        if m:
            root_sites.setdefault(m.group(2), []).append(int(m.group(1)))
    apps, libs = [], []
    for line in scr:
        if line.startswith('!'):
            break
        f = [x.strip() for x in line.rstrip(';').split(',')]
        if len(f) >= 4 and f[0] == 'CFE_APP':
            apps.append((f[3], f[2], f[1]))
        elif len(f) >= 4 and f[0] == 'CFE_LIB':
            libs.append((f[3], f[2], f[1]))

    t_init = t_stop = None
    ev = {}   # actor -> [(aseq, site, t)]
    losses = []
    for line in dump:
        m = re.match(r'run .*\bt_init=(\d+) .*\bt_stop=(\d+)', line)
        if m:
            t_init, t_stop = int(m.group(1)), int(m.group(2))
        m = re.match(r'ev actor=(\d+) aseq=(\d+) site=(\d+)\S* .*\bt=(\d+) ', line)
        if m:
            ev.setdefault(int(m.group(1)), []).append((int(m.group(2)), int(m.group(3)), int(m.group(4))))
        m = re.match(r'actor (\d+) .*\boverflow=(\d+) .*\bdropped_bytes=(\d+)', line)
        if m and (int(m.group(2)) or int(m.group(3))):
            losses.append('actor %s overflow=%s dropped_bytes=%s' % m.groups())
    if t_init is None:
        sys.stderr.write('stop_eval.py: no RUN line with t_init and t_stop in the dump text\n')
        return 2
    for k in ev:
        ev[k].sort()

    def ms(t):
        return '%.3f' % ((t - t_init) / 1e6)

    out = ['# stop_eval.py --manifest %s --sidecar %s --startup-script %s --dump-text %s' % (a.manifest, a.sidecar, a.startup_script, a.dump_text),
           '# stop APIs (sidecar): ' + '; '.join('%s %s %s' % (r, api, src) for api, (r, src) in sorted(apis.items())),
           '# stop sites (manifest): ' + ', '.join('%d=%s' % (s, api) for s, api in sorted(stop_sites.items())),
           '# window: t_init=%d t_stop=%d length_ms=%s; times below are ms after t_init' % (t_init, t_stop, ms(t_stop)),
           '# record losses: ' + ('; '.join(losses) if losses else 'none in the actor records'),
           'app\tentry\tfile\troot_sites\tactor\troot_ms\tstop_event\tstop_site\tstop_aseq\tstop_ms\tinside_window\tstatus']
    reached = 0
    for name, entry, fil in apps:
        rs = root_sites.get(entry, [])
        if not rs:
            out.append('\t'.join([name, entry, fil, '-', '-', '-', '-', '-', '-', '-', '-', 'unresolved(no root site in the plan)']))
            continue
        actors = [(k, e) for k, lst in ev.items() for e in lst if e[1] in rs]
        if not actors:
            out.append('\t'.join([name, entry, fil, ','.join(map(str, rs)), '-', '-', '-', '-', '-', '-', '-', 'unresolved(no root entry recorded)']))
            continue
        for k, root in sorted(set((k, min(e for kk, e in actors if kk == k)) for k, _ in actors)):
            stop = next((e for e in ev[k] if e[0] > root[0] and e[1] in stop_sites), None)
            if stop is None:
                last = ev[k][-1]
                out.append('\t'.join([name, entry, fil, ','.join(map(str, rs)), str(k), ms(root[2]), '-', '-', '-', '-', '-',
                                      'not-reached(last event aseq %d site %d at %s ms)' % (last[0], last[1], ms(last[2]))]))
                continue
            inside = 'yes' if t_init <= stop[2] <= t_stop else 'no'
            if inside == 'yes':
                reached += 1
            out.append('\t'.join([name, entry, fil, ','.join(map(str, rs)), str(k), ms(root[2]), stop_sites[stop[1]], str(stop[1]),
                                  str(stop[0]), ms(stop[2]), inside, 'reached' if inside == 'yes' else 'reached-outside-window']))
    for name, entry, fil in libs:
        out.append('\t'.join([name, entry, fil, '-', '-', '-', '-', '-', '-', '-', '-', 'library(no stop event by the rule)']))
    out.append('summary\tapplications=%d reached_inside_window=%d libraries=%d' % (len(apps), reached, len(libs)))
    text = '\n'.join(out) + '\n'
    if a.out:
        with open(a.out, 'w', encoding='utf-8') as f:
            f.write(text)
    sys.stdout.write(text)
    return 0 if apps and reached == len(apps) else 3


if __name__ == '__main__':
    sys.exit(main())
