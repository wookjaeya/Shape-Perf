#!/bin/sh
# Analyzer unit tests. The traces are tool-test inputs (unit_t.c uses fixed
# usleep values and a 20 ms witness delay); they are NOT research conditions
# (CONDITIONS_POLICY.md, last section).
set -e
d=$(dirname "$0"); A="$d/../cord_analyze.py"
check() { out=$(python3 "$A" "$d/$1" --all | tail -1); [ "$out" = "groups: $2" ] && echo "ok   $1" || { echo "FAIL $1: $out"; exit 1; }; }
check unit_benign.jsonl  "{'predicted_violation': 1, 'ordered': 1}"
check unit_witness.jsonl "{'observed_violation': 1, 'ordered': 1}"
check unit_bsem.jsonl    "{'ordered': 2}"
