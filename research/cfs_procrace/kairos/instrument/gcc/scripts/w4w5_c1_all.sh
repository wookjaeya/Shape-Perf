#!/usr/bin/env bash
# w4w5_c1_all.sh <out root> -- W4/W5 (2026-10-28) C1: every preserved and fresh C1 record read by w4w5_c1_replay.sh with
# the fixed analysis configuration (closures of w4w5_c1_closures.sh under <out root>/closures).  Two jobs at a time.
# Sets (<out root>/replay/<set>/<run>/):
#   p_f0tx  procrace/c1_runs/smoke3/<cfg>_<cand>              F0 natural tx (preserved, kernel v153)
#   p_f0rx  procrace/c1_runs/confirm1/N_<cfg>_<cand>_rx       F0 natural rx
#   p_gi    instr/gcc/d3/c1/<cfg>_<cand>_<inst>               G-I natural (stage D, build d2)
#   p_f0ctl procrace/c1_runs/confirm2/<W>_<cfg>_<cand>_<inst> F0 controlled (+ hold.kzr)
#   p_gih   instr/gcc/f1/gih/<W>_<cfg>_<cand>_<inst>          G-I-H controlled (stage F, build f1)
#   w_*     the W5 counterparts under w5/run (kernel v180): c1nat/f0tx, c1nat/f0rx, c1nat/gi (build f1), c1ctl/f0, c1ctl/gih
# Writes <out root>/replay/runs.log (one line per record: kind, run, the exit code of every step).
set -uo pipefail
O=${1:?out root}; mkdir -p "$O/replay"; O="$(cd "$O" && pwd)"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PR=/home/user/work/procrace/c1_runs; IG=/home/user/work/instr/gcc; W5=/home/user/work/w5/run
{
  for r in $PR/smoke3/{pre,post}_{1df262a,1b0b338};     do echo "f0nat $r p_f0tx"; done
  for r in $PR/confirm1/N_{pre,post}_{1df262a,1b0b338}_rx; do echo "f0nat $r p_f0rx"; done
  for r in $IG/d3/c1/*_*_*;                             do echo "gid2 $r p_gi"; done
  for r in $PR/confirm2/W*_*_*_*;                        do echo "f0ctl $r p_f0ctl"; done
  for r in $IG/f1/gih/W*_*_*_*;                          do echo "gif1 $r p_gih"; done
  for r in $W5/c1nat/f0tx/*_*;                           do echo "f0nat $r w_f0tx"; done
  for r in $W5/c1nat/f0rx/N_*;                           do echo "f0nat $r w_f0rx"; done
  for r in $W5/c1nat/gi/*_*_*;                           do echo "gif1 $r w_gi"; done
  for r in $W5/c1ctl/f0/W*_*_*_*;                        do echo "f0ctl $r w_f0ctl"; done
  for r in $W5/c1ctl/gih/W*_*_*_*;                       do echo "gif1 $r w_gih"; done
} | while read -r k r s; do [ -d "$r" ] && echo "$k $r $O/replay/$s/$(basename $r) $O/closures"; done \
  | xargs -P ${KZ_JOBS:-2} -L 1 "$HERE/w4w5_c1_replay.sh" | sort > "$O/replay/runs.log"
echo "records $(wc -l < "$O/replay/runs.log"); steps not 0: $(grep -cE ' [1-9][0-9]* ' "$O/replay/runs.log")"
