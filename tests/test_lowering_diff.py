"""Prediction and comparison helpers for the transpose-unroll contrast (design amendment v3)."""
from shapeperf import lowering_diff as D


def test_unroll_factor_mirrors_the_compiler_helper():
    # getNoLeftoverUnrollFactor: largest u in [8..2] dividing a literal trip count, else 1
    assert [D.no_leftover_unroll(L) for L in (41, 42, 44, 45, 46, 48, 49, 51, 54, 64, 149, 150, 256)] == \
        [1, 7, 4, 5, 2, 8, 7, 3, 6, 8, 1, 6, 8]
    assert D.no_leftover_unroll(48, cap=1) == 1          # arm S1: cap 1 never unrolls
    assert D.no_leftover_unroll(49, cap=1) == 1
    assert D.no_leftover_unroll(49, cap=4) == 1 and D.no_leftover_unroll(48, cap=4) == 4


def test_count_unrolled_loops_matches_trip_count_and_step_only():
    ir = """
      affine.for %arg7 = 0 to 49 step 7 {
      affine.for %arg8 = 0 to 49 {
      affine.for %a = 0 to 49 step 7 {
      affine.for %b = 0 to 64 step 7 {
      affine.for %c = 0 to 49 step 4 {
    """
    assert D.count_unrolled_loops(ir, 49, 7) == 2
    assert D.count_unrolled_loops(ir, 49, 4) == 1
    assert D.count_unrolled_loops(ir, 49, 8) == 0


def test_normalization_ignores_ssa_numbers_and_map_numbering():
    a = "#map3 = affine_map<(d0) -> (d0 + 1)>\n%12 = affine.load %arg[%i, 5]\n"
    b = "#map9 = affine_map<(d0) -> (d0 + 7)>\n%77 = affine.load %arg[%j, 9]\n"
    assert D.normalized_lines(a) == D.normalized_lines(b)
    assert D.diff_hunks(a, b) == {"hunks": 0, "changed_lines": 0}
    c = b + "affine.store %x, %y[%i]\n"
    assert D.diff_hunks(a, c) == {"hunks": 1, "changed_lines": 1}
