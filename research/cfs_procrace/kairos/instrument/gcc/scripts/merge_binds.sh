#!/usr/bin/env bash
# merge_binds.sh <binding dir> <out.kzs> [<dumpbase pattern>] -- one "kzs-sites 1" binding file from the plugin's
# per-unit files (bind lines and their '#' notes).  With a pattern, only units whose dumpbase matches it
# (e.g. to leave out unit-test objects).  Units the plugin left alone because they are unit-test coverage builds
# (skip-coverage) are not part of the run and are only counted.  Refuses duplicate binding ids (a unit compiled
# twice into one run).
set -uo pipefail
D=${1:?usage}; O=${2:?usage}; PAT=${3:-}
{
  echo "kzs-sites 1"
  echo "# merged from $D ($(ls "$D"/*.kzs | wc -l) unit files) on $(date -u +%FT%TZ)"
  for f in "$D"/*.kzs; do
    if [ -n "$PAT" ] && ! grep -q "^# .* dumpbase=.*$PAT" "$f"; then continue; fi
    if grep -q '^# note not instrumented: unit compiled with -fprofile-arcs' "$f"; then echo "# skipped (coverage unit, not instrumented): $(sed -n 's/.* dumpbase=//p' "$f" | head -1)"; continue; fi
    echo "# ---- $(basename "$f")"
    grep -v '^kzs-sites ' "$f"
  done
} > "$O"
dups=$(grep '^bind ' "$O" | awk '{print $2}' | sort | uniq -d)
[ -z "$dups" ] || { echo "duplicate binding ids: $dups" >&2; exit 1; }
echo "$O: $(grep -c '^bind ' "$O") bindings ($(grep '^bind ' "$O" | sed 's/.* status=\([a-z]*\).*/\1/' | sort | uniq -c | paste -sd' '))"
