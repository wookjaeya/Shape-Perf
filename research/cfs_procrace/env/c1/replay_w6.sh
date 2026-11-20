#!/usr/bin/env bash
# replay_w6.sh -- C1 W6 (2026-10-29): every C1 W6 record read with ONE fixed analysis configuration, the path
# comparisons of results_w4w5_20261028.md (kz occ diff of matched sets) and the PD29 D8 check against the campaign's
# inline replays.  Pilot/dev and controlled records only; nothing is run but kz-static (step 1), kz and text tools.
# No cFS boot.
#
#   replay_w6.sh bprime     B' of the four stage F derived builds (instr/gcc/f1/c1/<cfg>_<cand>), tx and rx, with the
#                           current profiles -> /home/user/work/w6/bprime_f1/<cfg>_<cand>_<inst>/
#   replay_w6.sh config     the fixed configuration record (repo, kz, kz-static, profile commit/tree/file hashes,
#                           closure and B' hashes) -> <out>/config.txt
#   replay_w6.sh replay     every C1 W6 record (list below) through "one", two jobs at a time -> <out>/replay/runs.log;
#                           the refused G-I-H calls -> <out>/replay/refused.tsv
#   replay_w6.sh one <kind> <record> <out dir>    one record (kinds f0nat, f0ctl, gif1 of w4w5_c1_replay.sh)
#   replay_w6.sh d8diag     the D8 decomposition (F0 records only): each record's inline replay command again
#                           (observe_c1_smoke.sh L167 / observe_c1_hold.sh L191: run.kzt as recorded, the four profiles,
#                           the two constants, no --closure, --compare-narrowing, --out) with the same kz and the
#                           current profiles -> <out>/d8diag/<run>/cur.{kz,txt}, occ_cur.txt.  Not a reading of the
#                           fixed configuration: it only separates the profile change (D8) from the closure and
#                           control-input changes between the inline replay and the fixed-configuration cur
#   replay_w6.sh occ        kz occ diff pairs -> <out>/occ/<group>/<name>__<reading>.txt, <out>/occ/pairs.tsv
#   replay_w6.sh tables     TSV summaries (python3 -I; reads only the outputs above and the records' text files)
#                           -> <out>/tables/*.tsv
#   replay_w6.sh all        bprime config replay d8diag occ tables
# <out> = /home/user/work/w6/replay/c1.
#
# Why not w4w5_c1_replay.sh itself: it does not take the W6 paths unchanged.  (a) Its F0 closure path is
# <closure root>/f0/<cfg>_<cand>[_rx] (L40); the W6 closures are /home/user/work/w6/closures/<cfg>_<cand>/c1prof[_rx]
# (plan/predict_w6_20261029.md section 1.1).  (b) Its record-name parsing (L28) strips only N_ and W*_ prefixes, so the
# stage B/C records B_post_* and C_pre_* stop with "bad record name" (exit 64).  (c) Its F0 class step (L80) reads
# /home/user/work/procrace/c1_runs/confirm2/scripts/classify.awk, which is not in the rebuilt environment
# (predict_w6_20261029.md section 6.2; c1_runs holds prov_roots only).
# So "one" below is w4w5_c1_replay.sh L23-L89 with these changes only:
#   - L28: also strips a one-letter plan tag (B_, C_);
#   - L40: CL=/home/user/work/w6/closures/<cfg>_<cand>/c1prof<SUF> (tx c1prof, rx c1prof_rx);
#   - L53: BP=/home/user/work/w6/bprime_f1/<cfg>_<cand>_<inst> (made by "bprime");
#   - L77-L82: the class of every record (F0 included) by classify_gih.awk, the notation the task asks for; it reads
#     the F0 trace as it is (CF and BP are the F0 actor names; checked on W1_post_1df262a_tx: "BP0_tsem: -17x25 ;
#     ends: CF=x"); the confirm2 reading-A notes of classify.awk are therefore absent [deviation, reference missing].
#   Everything else -- the four profiles and the two constants (L33-L34), the F0 basis choice (L39), kz convert --hold
#   (L44), the manifest check against run.meta (L55-L56), kz convert --kzrt with the manifest and the binding list and
#   no --process for f1 (L57-L59), the control test (L60), the replay arguments (L46, L61), the readings cur, p1off,
#   nocode, and ctloff_p1off, ctloff for records with control facts (L64-L75), --emit-trace (L76), control_cmp.sh
#   (L84-L87), sha256.txt (L88) and the one-line status (L89) -- is copied unchanged.
# "bprime" is the bp() of w4w5_c1_closures.sh (L36-L41) for stage f1, with the output root /home/user/work/w6, and its
# xargs line (L45, KZ_JOBS default 2); basis_gi.sh is called unchanged (KZ_PROFILES unset = kairos/profiles).
# "occ" pairs as w4w5_c1_occ.sh path_w5 (L56-L57 natural: F0 tx with G-I tx, F0 rx with G-I rx, readings cur p1off
# nocode; L74 controlled: F0+NCTL with G-I-H, five readings) and adds the D8 check pairs: group d8 (the new cur, and
# for controlled records also the new ctloff, against the record's inline replay.kz), d8prof (d8diag against the
# inline replay: the same command and kz, only the profiles differ) and d8rest (the new cur / ctloff against d8diag:
# the closure and, for controlled records, the realized control as NCTL).  It does not use the awk of
# w4w5_c1_occ.sh L89-L97, which reads header phrases the 2026-10-29 kz occ diff no longer prints
# (kairos/docs/replay_output.md section 4); "tables" parses the new header lines (pairs:, result:).
# Records (W6 bundle records/w6-20261029-runs2): F0 natural tx 4 (c1nat/f0tx), rx 4 (c1nat/f0rx), G-I natural 8
# (c1nat/gi, build f1), F0 controlled 16 (c1ctl/f0: B_post, C_pre with the recorded stage B/C plans; W1_post, W3_pre
# with the W6 predict plans), G-I-H 4 (c1ctl/gih/W1_post_*); c1ctl/gih/{B_,C_,W3_}* were refused by kz rtplan and
# have no boot: only their refused.txt is listed.
# Harness choices (unspecified_by_reference): the output layout, the d8 pair group, the TSV columns.
set -uo pipefail
STEP=${1:?bprime|config|replay|one|d8diag|occ|tables|all}
R=/home/user/Shape-Perf/research/cfs_procrace
K=$R/kairos
S=$K/instrument/gcc/scripts
KZ_BIN=${KZ_BIN:-/home/user/work/kairos-build/tools/kz/kz}
KZ_STATIC=${KZ_STATIC:-/home/user/work/kairos-build/k1r3-llvm/tools/kz-static/kz-static}
P=$K/profiles/c1-bp-b051eb0
W6=/home/user/work/w6
RUN=$W6/run
CLR=$W6/closures
BPR=$W6/bprime_f1
OUT=$W6/replay/c1
I=/home/user/work/instr/gcc
SELF="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/$(basename "${BASH_SOURCE[0]}")"
wait_flag() { while [ -e /home/user/work/.campaign_running ]; do sleep 60; done; }

case "$STEP" in
bprime)
  wait_flag; mkdir -p "$BPR"
  bp() {  # w4w5_c1_closures.sh L36-L41, stage f1, output root /home/user/work/w6
    local S1=f1 CFG=$1 CAND=$2 INST=$3
    local W=$I/$S1/c1/${CFG}_${CAND} D=$BPR/${CFG}_${CAND}_$INST
    KZ_STATIC=$KZ_STATIC "$S/basis_gi.sh" c1 $W $CFG $CAND $INST $D | tr '\n' ' '
    sha256sum $W/ws/build/native/default/compile_commands.json $W/binds_instr.kzs >> $D/sha256.txt; echo " ($S1)"
  }
  export -f bp; export S I BPR KZ_STATIC
  { for CFG in pre post; do for CAND in 1df262a 1b0b338; do for INST in tx rx; do echo "bp $CFG $CAND $INST"; done; done; done; } \
    | xargs -P ${KZ_JOBS:-2} -L 1 bash -c '"$@"' _ | sort > "$BPR/bprime.log"
  cat "$BPR/bprime.log"
  { echo "# replay_w6.sh bprime $(date -u +%FT%T.%3NZ) repo $(git -C $R rev-parse HEAD) diff_kairos_lines=$(git -C $R diff HEAD -- kairos | wc -l)"
    cat "$BPR"/*/sha256.txt; } > "$BPR/sha256.txt" ;;
config)
  mkdir -p "$OUT"
  { echo "# replay_w6.sh config $(date -u +%FT%T.%3NZ)"
    echo "repo $(git -C $R rev-parse HEAD) diff_kairos_lines=$(git -C $R diff HEAD -- kairos | wc -l)"
    echo "kz $KZ_BIN sha256=$(sha256sum $KZ_BIN | cut -c1-64)"
    echo "kz-static $KZ_STATIC sha256=$(sha256sum $KZ_STATIC | cut -c1-64)"
    echo "profiles $P last_commit=$(git -C $R log -1 --format=%H -- kairos/profiles/c1-bp-b051eb0) tree=$(git -C $R rev-parse HEAD:research/cfs_procrace/kairos/profiles/c1-bp-b051eb0) uncommitted_lines=$(git -C $R diff HEAD -- kairos/profiles | wc -l)"
    echo "constants CFE_PLATFORM_ES_MAX_APPLICATIONS=32 OS_MAX_API_NAME=20 (results_pilot_20261025.md section 0)"
    echo "## profile files"; (cd $K/profiles && find c1-bp-b051eb0 -type f | sort | xargs sha256sum)
    echo "## F0 closures: sha256sum -c $CLR/sha256.txt"; grep -v '^#' $CLR/sha256.txt | sha256sum -c - 2>&1
    echo "## B' f1: sha256sum -c of the closure lines of $BPR/*/sha256.txt"; cat $BPR/*/sha256.txt | sha256sum -c - 2>&1
  } > "$OUT/config.txt"; cat "$OUT/config.txt" | sed -n '1,6p' ;;
one)
  # w4w5_c1_replay.sh L23-L89 (changes listed in the header)
  KIND=${2:?kind}; RR=${3:?record}; O=${4:?out}
  b=$(basename "$RR"); x=${b#N_}; case "$x" in W*|[A-Z]_*) x=${x#*_} ;; esac
  CFG=${x%%_*}; CAND=$(echo "$x" | cut -d_ -f2); INST=$(echo "$x" | cut -d_ -f3); [ -n "$INST" ] || INST=tx
  case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "bad record name $b" >&2; exit 64 ;; esac
  SUF=; [ "$INST" = rx ] && SUF=_rx
  mkdir -p "$O"; : > "$O/exit.txt"
  PROF=(--profile $P --profile $P/cf-$REV --profile $P/bundle-$CAND --profile $P/rspec-$REV
        --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20)
  IN=(); CTL=0
  case "$KIND" in
    f0nat|f0ctl)
      BAS=$RR/basis_core.kz; [ -f "$RR/basis_core-$INST.kz" ] && BAS=$RR/basis_core-$INST.kz
      CL=$CLR/${CFG}_${CAND}/c1prof$SUF
      T=$RR/run.kzt; IN+=("$RR/run.kzt" "$BAS")
      if [ "$KIND" = f0ctl ]; then
        CTL=1; T=$O/run.kzt; IN+=("$RR/hold.kzr")
        $KZ_BIN convert "$RR/run.kzt" --hold "$RR/hold.kzr" --out "$T" > "$O/convert.txt" 2>&1; echo "convert $?" >> "$O/exit.txt"
      fi
      A=("$T" "${PROF[@]}" --basis "$BAS" --facts $CL/closure.facts.kz --static-basis $CL/closure.basis.kz)
      IN+=($CL/closure.facts.kz $CL/closure.basis.kz) ;;
    gif1)
      W=$I/f1/c1/${CFG}_${CAND}; MAN=$P/instr/sites-cf$REV.kzs; BP=$BPR/${CFG}_${CAND}_$INST; PROC=()
      want=$(sed -n 's/^manifest .* sha256=\([0-9a-f]*\)$/\1/p' "$RR/run.meta" | head -1); have=$(sha256sum "$MAN" | cut -c1-64)
      [ "$want" = "$have" ] || { echo "manifest mismatch: run.meta $want, $MAN $have" >> "$O/exit.txt"; exit 65; }
      T=$O/run.kzt; IN+=("$RR/dump/$INST.kzrt" "$MAN" "$W/binds_instr.kzs" $BP/basis.kz $BP/facts.kz)
      $KZ_BIN convert --kzrt "$RR/dump/$INST.kzrt" --manifest "$MAN" --manifest "$W/binds_instr.kzs" --basis $BP/basis.kz "${PROC[@]}" \
          --out "$T" > "$O/convert.txt" 2>&1; echo "convert $?" >> "$O/exit.txt"
      grep -q 'control records [1-9]' "$O/convert.txt" && CTL=1
      A=("$T" "${PROF[@]}" --basis $BP/basis.kz --facts $BP/facts.kz --static-basis $BP/basis.kz) ;;
    *) echo "usage: replay_w6.sh one f0nat|f0ctl|gif1 <record> <out>" >&2; exit 64 ;;
  esac
  rd() {  # name args...
    local n=$1; shift
    $KZ_BIN replay "${A[@]}" "$@" --out "$O/$n.kz" > "$O/$n.txt" 2>&1; echo "$n $?" >> "$O/exit.txt"
    $KZ_BIN occ show "$O/$n.kz" > "$O/occ_$n.txt" 2>&1; echo "occ_$n $?" >> "$O/exit.txt"
  }
  rd cur --compare-narrowing
  rd p1off --outcome-narrowing off
  rd nocode --outcome-narrowing off --reads-from-outcome off
  if [ $CTL = 1 ]; then
    rd ctloff_p1off --control-order off --outcome-narrowing off
    rd ctloff --control-order off
  fi
  $KZ_BIN replay "${A[@]}" --emit-trace > "$O/trace.txt" 2>&1; echo "trace $?" >> "$O/exit.txt"
  awk -f "$S/classify_gih.awk" "$O/trace.txt" > "$O/class.txt" 2>&1
  echo "classify $?" >> "$O/exit.txt"
  if [ $CTL = 1 ]; then
    case "$KIND" in f0ctl) "$S/control_cmp.sh" f0 "$RR" ;; *) "$S/control_cmp.sh" gih "$RR" ;; esac > "$O/control.txt" 2>&1
    echo "control $?" >> "$O/exit.txt"
  fi
  sha256sum "${IN[@]}" "$O"/*.kz "$O"/*.txt "$KZ_BIN" $( [ -f "$O/run.kzt" ] && echo "$O/run.kzt") 2>/dev/null | grep -v '/sha256.txt$' > "$O/sha256.txt"
  echo "$KIND $b $(tr '\n' ' ' < "$O/exit.txt")" ;;
replay)
  wait_flag; mkdir -p "$OUT/replay"
  {
    for r in $RUN/c1nat/f0tx/{pre,post}_{1df262a,1b0b338}; do echo "f0nat $r f0tx"; done
    for r in $RUN/c1nat/f0rx/N_*;                     do echo "f0nat $r f0rx"; done
    for r in $RUN/c1nat/gi/*_*_*;                     do echo "gif1 $r gi"; done
    for r in $RUN/c1ctl/f0/{B_post,C_pre,W1_post,W3_pre}_*; do echo "f0ctl $r f0ctl"; done
    for r in $RUN/c1ctl/gih/W1_post_*;                do echo "gif1 $r gih"; done
  } | while read -r k r s; do [ -d "$r" ] && echo "$k $r $OUT/replay/$s/$(basename $r)"; done \
    | xargs -P ${KZ_JOBS:-2} -L 1 "$SELF" one | sort > "$OUT/replay/runs.log"
  printf 'run\tplan_file\treason\n' > "$OUT/replay/refused.tsv"
  for d in $RUN/c1ctl/gih/{B_,C_,W3_}*; do
    [ -f "$d/refused.txt" ] || { printf '%s\t-\tNO refused.txt\n' "$(basename $d)" >> "$OUT/replay/refused.tsv"; continue; }
    printf '%s\t%s\t%s\n' "$(basename $d)" "$(sed -n 's/.*kz rtplan: \([^:]*\.kzh\): .*/\1/p' $d/refused.txt | head -1)" "$(head -1 $d/refused.txt)" >> "$OUT/replay/refused.tsv"
  done
  echo "records $(wc -l < "$OUT/replay/runs.log"); steps not 0: $(grep -cE ' [1-9][0-9]* ' "$OUT/replay/runs.log"); refused $(($(wc -l < "$OUT/replay/refused.tsv") - 1))" ;;
d8diag)
  wait_flag; mkdir -p "$OUT/d8diag"
  for RR in $RUN/c1nat/f0tx/{pre,post}_* $RUN/c1nat/f0rx/N_* $RUN/c1ctl/f0/{B_post,C_pre,W1_post,W3_pre}_*; do
    b=$(basename "$RR"); x=${b#N_}; case "$x" in W*|[A-Z]_*) x=${x#*_} ;; esac
    CFG=${x%%_*}; CAND=$(echo "$x" | cut -d_ -f2)
    case "$CFG" in pre) REV=281a941 ;; post) REV=833fdbb ;; *) echo "bad record name $b" >&2; exit 64 ;; esac
    O=$OUT/d8diag/$b; mkdir -p "$O"
    # observe_c1_smoke.sh L167 = observe_c1_hold.sh L191 with CUR=() (the campaign gave no --closure)
    $KZ_BIN replay "$RR/run.kzt" --profile "$P" --profile "$P/cf-$REV" --profile "$P/bundle-$CAND" --profile "$P/rspec-$REV" \
        --const CFE_PLATFORM_ES_MAX_APPLICATIONS=32 --const OS_MAX_API_NAME=20 --compare-narrowing \
        --out "$O/cur.kz" > "$O/cur.txt" 2>&1; e1=$?
    $KZ_BIN occ show "$O/cur.kz" > "$O/occ_cur.txt" 2>&1; e2=$?
    sha256sum "$RR/run.kzt" "$P/rspec-$REV/40_rspec.kzp" "$O/cur.kz" "$O/cur.txt" "$KZ_BIN" > "$O/sha256.txt"
    echo "d8diag $b cur $e1 occ_cur $e2"
  done | tee "$OUT/d8diag/runs.log" ;;
occ)
  RP=$OUT/replay; OC=$OUT/occ; mkdir -p "$OC"; : > "$OC/pairs.txt"
  pair() { echo "$1 $2 $3 $4 $5" >> "$OC/pairs.txt"; }   # group A.kz B.kz name reading
  for c in pre post; do for d in 1df262a 1b0b338; do
    for r in cur p1off nocode; do   # w4w5_c1_occ.sh L56-L57
      pair path_nat $RP/f0tx/${c}_$d/$r.kz $RP/gi/${c}_${d}_tx/$r.kz nat_${c}_${d}_tx $r
      pair path_nat $RP/f0rx/N_${c}_${d}_rx/$r.kz $RP/gi/${c}_${d}_rx/$r.kz nat_${c}_${d}_rx $r
    done
    for x in "f0tx/${c}_$d c1nat/f0tx/${c}_$d" "f0rx/N_${c}_${d}_rx c1nat/f0rx/N_${c}_${d}_rx"; do set -- $x; b=$(basename $1)
      pair d8 $RP/$1/cur.kz $RUN/$2/replay.kz $b cur=inline
      pair d8prof $OUT/d8diag/$b/cur.kz $RUN/$2/replay.kz $b d8diag=inline
      pair d8rest $RP/$1/cur.kz $OUT/d8diag/$b/cur.kz $b cur=d8diag
    done
  done; done
  for D in $RP/f0ctl/*; do b=$(basename $D)
    case "$b" in W1_post_*) for r in cur p1off nocode ctloff_p1off ctloff; do pair path_ctl $D/$r.kz $RP/gih/$b/$r.kz ctl_$b $r; done ;; esac   # w4w5_c1_occ.sh L74
    pair d8 $D/cur.kz $RUN/c1ctl/f0/$b/replay.kz $b cur=inline
    pair d8 $D/ctloff.kz $RUN/c1ctl/f0/$b/replay.kz $b ctloff=inline
    pair d8prof $OUT/d8diag/$b/cur.kz $RUN/c1ctl/f0/$b/replay.kz $b d8diag=inline
    pair d8rest $D/cur.kz $OUT/d8diag/$b/cur.kz $b cur=d8diag
    pair d8rest $D/ctloff.kz $OUT/d8diag/$b/cur.kz $b ctloff=d8diag
  done
  : > "$OC/pairs.tsv"
  while read -r g a bb n r; do
    mkdir -p "$OC/$g"; f=$OC/$g/${n}__$r.txt
    $KZ_BIN occ diff "$a" "$bb" > "$f" 2>&1; rc=$?
    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\n' "$g" "$n" "$r" "$rc" "$a" "$bb" "$f" >> "$OC/pairs.tsv"
  done < "$OC/pairs.txt"
  awk -F'\t' '{ k = $1 " " $3; n[k]++; c[k, $4]++; if (!($4 in seen)) { seen[$4] = 1; codes[++nc] = $4 } }
    END { for (k in n) { s = ""; for (i = 1; i <= nc; i++) if (c[k, codes[i]]) s = s " exit" codes[i] "=" c[k, codes[i]]; print k " pairs " n[k] s } }' "$OC/pairs.tsv" | sort ;;
tables)
  # Tables (the parsing of w4w5_c1_tables.sh L29-L50, L94-L100, L128-L139; edge rules keep their counts):
  #   records.tsv     per record: steps, loss, events, occurrences, class (classify_gih.awk), CF's end, lookups per key,
  #                   CF console events (30 = "failed to get sem id", 20 = "CF Initialized"), BP 15, realized control
  #   occ_rows.tsv    per record, reading and occurrence (kz occ show)
  #   judgements.tsv  per record and reading: (CF,<key>,<ordinal range>) status summary {basis kinds}
  #   classes.tsv     the same grouped by set, plan_cfg and reading
  #   withheld.tsv    controlled records: INCONCLUSIVE(horizon-open) of ctloff_p1off and how many p1off judges
  #   occ.tsv         per kz occ diff: rc, counts of the header lines (occurrences, pairs:, result:), differences
  #   occ_lines.tsv   every difference line;  occ_join.tsv: both sides of every pair joined on (key name, ordinal)
  #   d8.tsv          the d8, d8prof, d8rest pairs with the inline replay's commit (D8 in it or not) and closure use
  mkdir -p "$OUT/tables"
  python3 -I - "$OUT" "$RUN" "$R" <<'PY'
import os, re, sys, glob, collections, subprocess
O, RUN, REPO = sys.argv[1], sys.argv[2], sys.argv[3]
RP = O + "/replay"; OC = O + "/occ"; T = O + "/tables"; os.makedirs(T, exist_ok=True)
SRC = {"f0tx": "c1nat/f0tx", "f0rx": "c1nat/f0rx", "gi": "c1nat/gi", "f0ctl": "c1ctl/f0", "gih": "c1ctl/gih"}
READINGS = ("cur", "p1off", "nocode", "ctloff_p1off", "ctloff")
def rd(p):
    try: return open(p, errors="replace").read()
    except OSError: return ""
def parse_name(b):  # as "one": N_ and a plan tag (W<n>_, B_, C_) stripped
    x = b[2:] if b.startswith("N_") else b
    plan = "N"
    if re.match(r"^(W\d+|[A-Z])_", x): plan, x = x.split("_", 1)
    f = x.split("_"); inst = f[2] if len(f) > 2 else "tx"
    return plan, f[0], f[1], inst
def path_of(s): return "F0" if s.startswith("f0") else ("G-I-H" if s == "gih" else "G-I")
def edge_rules(e):  # "edges N r[k]#a->#b ..." -> sorted rule[kind] with counts, endpoints left out
    c = collections.Counter(re.sub(r"#\d+->#\d+", "", x) for x in e.split()[2:])
    return " ".join("%s%s" % (k, "x%d" % v if v > 1 else "") for k, v in sorted(c.items())) or "-"
def occ_rows(f):  # kz occ show rows (w4w5_c1_tables.sh occ_rows, edge counts kept)
    rows = []
    for line in rd(f).splitlines():
        if not line.startswith("#"): continue
        p = line.split(" | ")
        if len(p) < 10: continue
        st, sm = p[6].split(" ", 1)
        m = re.match(r"basis \{([^}]*)\} assume \{([^}]*)\}.* outcome (\S+)", p[7])
        name = re.sub(r"^args sem_name=", "", p[8]).split()[0] if "sem_name=" in p[8] else p[8]
        rows.append(dict(owner=p[2], res=p[3], ord=int(p[4]), exec=p[5], status=st, summary=sm,
                         basis=m.group(1) if m else "?", assume=m.group(2) if m else "?", outcome=m.group(3) if m else "?",
                         name=name, edges=edge_rules(p[9])))
    return rows
def short(n): return n.replace("_tsem", "")
recs = []
for s in ("f0tx", "f0rx", "gi", "f0ctl", "gih"):
    for d in sorted(glob.glob(RP + "/" + s + "/*/")):
        b = os.path.basename(d.rstrip("/")); R = RUN + "/" + SRC[s] + "/" + b
        plan, cfg, cand, inst = parse_name(b)
        meta = rd(R + "/run.meta") + rd(R + "/smoke.meta") + rd(R + "/hold.meta")
        k = re.search(r"kernel=(\S+)", meta); kernel = k.group(1) if k else "?"
        cur = rd(d + "cur.txt"); conv = rd(d + "convert.txt")
        loss = re.search(r"^loss .*$", cur, re.M); loss = loss.group(0) if loss else "?"
        ev = re.search(r"raw events (\d+), trace events (\d+)", cur)
        nl = re.search(r"neutral layer: .*events (\d+).*losses (\d+), control records (\d+)", conv)
        events = ("raw %s trace %s" % ev.groups()) if ev else ""
        if nl: events += "; neutral %s losses %s ctl %s" % nl.groups()
        rc = re.search(r"realized control: records (\d+), releases that resumed a held thread (\d+), control edges (\d+)", cur)
        cl = rd(d + "class.txt").splitlines(); cls = cl[0].replace("class: ", "") if cl else "?"
        ends = re.search(r"ends: CF=(\S)", cls); ends = ends.group(1) if ends else "?"
        look = {}
        for key, seq in re.findall(r"(\S+_tsem): ([^;]+)", cls):
            look[key] = sum(int(n) for n in re.findall(r"x(\d+)", seq))
        con = rd(R + "/inst/" + inst + "/console_output.log")
        cfev = collections.Counter(re.findall(r"/CF (\d+): ", con))
        cf_init = "yes" if re.search(r"/CF 20: CF Initialized", con) else "no"
        bp15 = len(re.findall(r"/BP 15: Created semaphore", con))
        ctl = [x for x in rd(d + "control.txt").split("\n") if x]
        exits = rd(d + "exit.txt").split(); bad = [exits[i] for i in range(0, len(exits) - 1, 2) if exits[i + 1] != "0"]
        rows = {r: occ_rows(d + "occ_" + r + ".txt") for r in READINGS if os.path.exists(d + "occ_" + r + ".txt")}
        drv = rd(R + "/driver.txt"); ks = re.search(r"kz.source: (?:repository )?commit ([0-9a-f]+)", drv)
        recs.append(dict(set=s, run=b, d=d, R=R, plan=plan, cfg=cfg, cand=cand, inst=inst, kernel=kernel, loss=loss, events=events,
                         nocc=len(rows.get("cur", [])), cls=cls, ends=ends, look=look, cf30=cfev.get("30", 0), cf_init=cf_init,
                         cfev=",".join("%s:%d" % kv for kv in sorted(cfev.items())), bp15=bp15, ctl=ctl,
                         rc=rc.groups() if rc else ("-", "-", "-"), bad=",".join(bad) or "ok", rows=rows,
                         source=ks.group(1) if ks else "-"))
with open(T + "/records.tsv", "w") as f:
    f.write("set\trun\tpath\tkernel\tcfg\tcand\tinst\tplan\tsteps\tloss\tevents\toccurrences\tclass\tCF_end\tlookups_BP0\tlookups_BP1\t"
            "CF_events\tCF30\tCF_Initialized\tBP15_created\tnctl_records\tnctl_resumed\tcontrol_edges\tcontrol_lines\n")
    for r in recs:
        f.write("\t".join(map(str, [r["set"], r["run"], path_of(r["set"]), r["kernel"], r["cfg"], r["cand"], r["inst"], r["plan"], r["bad"],
                r["loss"], r["events"], r["nocc"], r["cls"], {"x": "exit(CFE_ES_ExitApp)", "f": "CF_Init=0"}.get(r["ends"], r["ends"]),
                r["look"].get("BP0_tsem", 0), r["look"].get("BP1_tsem", 0), r["cfev"] or "-", r["cf30"], r["cf_init"], r["bp15"]]
                + list(r["rc"]) + [" | ".join(r["ctl"]) or "-"])) + "\n")
with open(T + "/occ_rows.tsv", "w") as f:
    f.write("set\trun\treading\tname\tordinal\towner\tstatus\tsummary\tbasis\tassume\toutcome\tedges\n")
    for r in recs:
        for rdg, rows in r["rows"].items():
            for o in rows:
                f.write("\t".join(map(str, [r["set"], r["run"], rdg, o["name"], o["ord"], o["owner"], o["status"], o["summary"], o["basis"],
                                            o["assume"], o["outcome"], o["edges"]])) + "\n")
def signature(rows):  # (key, ordinal range, status, summary, basis kinds), runs of equal rows joined
    out, prev = [], None
    for o in sorted(rows, key=lambda o: (o["name"], o["ord"])):
        k = (o["name"], o["status"], o["summary"], o["basis"])
        if prev and prev[0] == k and prev[2] == o["ord"] - 1: prev[2] = o["ord"]; continue
        prev = [k, o["ord"], o["ord"]]; out.append(prev)
    return " ; ".join("(CF,%s,%s) %s %s {%s}" % (k[0], str(a) if a == b else "%d-%d" % (a, b), k[1], k[2], k[3]) for k, a, b in out) or "(none)"
with open(T + "/judgements.tsv", "w") as f:  # per record and reading
    f.write("set\trun\tpath\tplan\tcfg\tinst\treading\tsignature\n")
    for r in recs:
        for rdg, rows in r["rows"].items():
            f.write("\t".join([r["set"], r["run"], path_of(r["set"]), r["plan"], r["cfg"], r["inst"], rdg, signature(rows)]) + "\n")
with open(T + "/classes.tsv", "w") as f:  # per set, plan_cfg, reading: the signature and the records that have it
    f.write("set\tgroup\treading\tn\trecords\tsignature\n")
    g = collections.OrderedDict()
    for r in recs:
        for rdg, rows in r["rows"].items():
            g.setdefault((r["set"], r["plan"] + "_" + r["cfg"], rdg, signature(rows)), []).append(r["cand"][:2] + "-" + r["inst"])
    for (s, grp, rdg, sig), inst in g.items(): f.write("%s\t%s\t%s\t%d\t%s\t%s\n" % (s, grp, rdg, len(inst), ",".join(inst), sig))
with open(T + "/withheld.tsv", "w") as f:  # w4w5_c1_tables.sh L128-L139
    f.write("set\trun\tplan\twithheld_ctloff_p1off\tjudged_p1off\tjudged_with_control_order\tremaining\tremaining_keys\tjudged_nocode\n")
    for r in recs:
        if "ctloff_p1off" not in r["rows"]: continue
        key = lambda o: (o["name"], o["ord"])
        w = [o for o in r["rows"]["ctloff_p1off"] if o["summary"].startswith("INCONCLUSIVE") and "horizon-open" in o["summary"]]
        p1 = {key(o): o for o in r["rows"]["p1off"]}; nc = {key(o): o for o in r["rows"]["nocode"]}
        j = [o for o in w if not p1[key(o)]["summary"].startswith("INCONCLUSIVE")]
        jc = [o for o in j if "control-order" in p1[key(o)]["basis"]]
        rem = [o for o in w if p1[key(o)]["summary"].startswith("INCONCLUSIVE")]
        jn = [o for o in w if not nc[key(o)]["summary"].startswith("INCONCLUSIVE")]
        f.write("\t".join(map(str, [r["set"], r["run"], r["plan"], len(w), len(j), len(jc), len(rem), ",".join("%s#%d" % key(o) for o in rem) or "-", len(jn)])) + "\n")
# kz occ diff outputs (the 2026-10-29 header lines; kairos/docs/replay_output.md section 4)
occ = []
for line in rd(OC + "/pairs.tsv").splitlines():
    g, n, rdg, rc, a, b, fn = line.split("\t"); t = rd(fn)
    oc = re.search(r"^occurrences A (\d+), B (\d+); matched (\d+), only in A (\d+), only in B (\d+)(.*)$", t, re.M)
    pr = re.search(r"^pairs: by the full key (\d+), with one side's owner not established (\d+), with neither side's owner established (\d+), by the text arguments (\d+)", t, re.M)
    res = re.search(r"^result: (.*)$", t, re.M); ce = re.search(r"^control edges A (\d+), B (\d+)", t, re.M)
    vd = sum(int(x) for x in re.findall(r"^verdict differences[^:]*: (\d+)", t, re.M))
    bd = sum(int(x) for x in re.findall(r"^basis differences[^:]*: (\d+)", t, re.M))
    occ.append(dict(g=g, n=n, r=rdg, rc=rc, a=a, b=b, f=fn, oa=oc.group(1) if oc else "?", ob=oc.group(2) if oc else "?",
                    m=oc.group(3) if oc else "?", ia=oc.group(4) if oc else "?", ib=oc.group(5) if oc else "?",
                    extra=(oc.group(6).strip("; ") if oc else "") or "-", full=pr.group(1) if pr else "?",
                    weak="%s/%s/%s" % pr.groups()[1:] if pr else "?", res=res.group(1) if res else "?",
                    ce="%s/%s" % ce.groups() if ce else "?", vd=vd, bd=bd,
                    lines=[x.strip() for x in t.splitlines() if x.startswith("  K-REG")]))
with open(T + "/occ.tsv", "w") as f:
    f.write("group\tname\treading\trc\tocc_a\tocc_b\tmatched\tonly_a\tonly_b\textra_counts\tpairs_full_key\tpairs_weak(one/neither/text)\t"
            "control_edges_a/b\tverdict_diff\tbasis_diff\tresult\n")
    for o in occ:
        f.write("\t".join(map(str, [o["g"], o["n"], o["r"], o["rc"], o["oa"], o["ob"], o["m"], o["ia"], o["ib"], o["extra"], o["full"], o["weak"],
                                    o["ce"], o["vd"], o["bd"], o["res"]])) + "\n")
with open(T + "/occ_lines.tsv", "w") as f:  # every difference line of every diff
    f.write("group\tname\treading\tline\n")
    for o in occ:
        for x in o["lines"]: f.write("%s\t%s\t%s\t%s\n" % (o["g"], o["n"], o["r"], x))
# per-occurrence join of the two sides of every pair on (key name, ordinal) from kz occ show (results_w4w5 section 4.3)
def show_rows(kz):  # the occ show text next to a replay output container; for an inline replay.kz, run kz occ show
    d, b = os.path.split(kz); n = b[:-3]
    p = d + "/occ_" + n + ".txt"
    if os.path.exists(p): return occ_rows(p)
    q = T + "/inline_show/" + d.strip("/").replace("/", "_") + ".txt"
    if not os.path.exists(q):
        os.makedirs(T + "/inline_show", exist_ok=True)
        with open(q, "w") as fh: subprocess.run([os.environ.get("KZ_BIN", "/home/user/work/kairos-build/tools/kz/kz"), "occ", "show", kz], stdout=fh, stderr=subprocess.STDOUT)
    return occ_rows(q)
with open(T + "/occ_join.tsv", "w") as f:
    f.write("group\tname\treading\tkey\tordinal\tA_status\tA_summary\tA_basis\tA_outcome\tA_edges\tB_status\tB_summary\tB_basis\tB_outcome\tB_edges\tverdict_same\tbasis_same\n")
    for o in occ:
        A = {(x["name"], x["ord"]): x for x in show_rows(o["a"])}; B = {(x["name"], x["ord"]): x for x in show_rows(o["b"])}
        for k in sorted(set(A) | set(B)):
            a, b = A.get(k), B.get(k)
            if not a or not b:
                f.write("\t".join(map(str, [o["g"], o["n"], o["r"], k[0], k[1]] + (["-"] * 5 if not a else [a["status"], a["summary"], a["basis"], a["outcome"], a["edges"]])
                                      + (["-"] * 5 if not b else [b["status"], b["summary"], b["basis"], b["outcome"], b["edges"]]) + ["unpaired", "unpaired"])) + "\n"); continue
            vs = a["status"] == b["status"] and a["summary"] == b["summary"]
            bs = a["basis"] == b["basis"] and a["assume"] == b["assume"] and a["outcome"] == b["outcome"] and a["edges"] == b["edges"]
            f.write("\t".join(map(str, [o["g"], o["n"], o["r"], k[0], k[1], a["status"], a["summary"], a["basis"], a["outcome"], a["edges"],
                                        b["status"], b["summary"], b["basis"], b["outcome"], b["edges"], "yes" if vs else "NO", "yes" if bs else "NO"])) + "\n")
# D8 check: the record's inline replay (campaign: observe_c1_smoke.sh / observe_c1_hold.sh, no --closure, the profile of
# its kz.source commit) against the new cur (and ctloff); the commit tells whether the inline replay read the D8 rspec
def has_d8(c):
    if c == "-": return "?"
    return "yes" if subprocess.run(["git", "-C", REPO, "merge-base", "--is-ancestor", "845eebc", c]).returncode == 0 else "no"
src = {r["run"]: r for r in recs if r["set"].startswith("f0")}
with open(T + "/d8.tsv", "w") as f:
    f.write("group\trecord\tcfg\tplan\tinline_commit\tinline_rspec_D8\tinline_closure\tpair(A=B)\trc\tverdict_diff\tbasis_diff\tB_signature\tA_signature\n")
    for o in occ:
        if not o["g"].startswith("d8"): continue
        r = src.get(o["n"])
        inl = rd(r["R"] + "/replay.txt") if r else ""
        clo = "none" if "no static closure given" in inl else "given"
        f.write("\t".join(map(str, [o["g"], o["n"], r["cfg"] if r else "?", r["plan"] if r else "?", r["source"][:7] if r else "?",
                                    has_d8(r["source"]) if r else "?", clo, o["r"], o["rc"], o["vd"], o["bd"],
                                    signature(show_rows(o["b"])), signature(show_rows(o["a"]))])) + "\n")
print("records %d; tables %s" % (len(recs), " ".join(sorted(os.path.basename(x) for x in glob.glob(T + "/*.tsv")))))
PY
  ;;
all)
  "$SELF" bprime && "$SELF" config && "$SELF" replay && "$SELF" d8diag && "$SELF" occ && "$SELF" tables ;;
*) echo "usage: replay_w6.sh bprime|config|replay|one|d8diag|occ|tables|all" >&2; exit 64 ;;
esac
