# census/ — evaluator only

Output of `scripts/run_census.py` (spec §11 G2.5): per-length probe signatures,
change points `C_sig`, alignment decomposition `B_align` / `C_nonalign`, the
normalization version, and raw IR for the lengths next to change points.

* Only `scripts/run_census.py` writes here and only `evaluate.py census` reads it.
* Selectors (`shapeperf/selectors/`) must never read it. Enforced by
  `tests/test_isolation.py` (static import/name check + runtime audit-hook
  sandbox that rejects any file access from inside a selector decision).
* `run_census.py` sets owner-only permissions (0700/0600). On the measurement VM,
  run selectors under a different UID for OS-level separation.
* Data files are git-ignored; keep them with the run's evidence bundle.
