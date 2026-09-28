"""Padding helpers keep valid tokens and only move trailing padding (spec §4.3)."""
import numpy as np
import pytest

from shapeperf import squad


def _feat():
    ids = np.array([[101, 7, 8, 102, 9, 102, 0, 0, 0, 0]], dtype=np.int64)
    mask = np.array([[1, 1, 1, 1, 1, 1, 0, 0, 0, 0]], dtype=np.int64)
    seg = np.array([[0, 0, 0, 0, 1, 1, 0, 0, 0, 0]], dtype=np.int64)
    return {"input_ids": ids, "input_mask": mask, "segment_ids": seg,
            "unique_id": np.array([3]), "valid_length": np.array([6])}


def test_trailing_padding_check():
    f = _feat()
    assert squad.check_trailing_padding(f["input_ids"], f["input_mask"], f["segment_ids"]) == []
    bad = f["input_mask"].copy()
    bad[0, 8] = 1
    assert squad.check_trailing_padding(f["input_ids"], bad, f["segment_ids"]) == [0]


@pytest.mark.parametrize("s", [6, 7, 10, 13])
def test_make_padded_inputs(s):
    f = _feat()
    x = squad.make_padded_inputs(f, 0, s)
    assert x["input_ids"].shape == (1, s)
    assert x["input_ids"][0, :6].tolist() == [101, 7, 8, 102, 9, 102]
    assert x["input_mask"][0].sum() == 6
    assert (x["input_ids"][0, 6:] == 0).all() and (x["segment_ids"][0, 6:] == 0).all()
    assert x["unique_ids"].tolist() == [3]


def test_cannot_cut_valid_tokens():
    with pytest.raises(ValueError):
        squad.make_padded_inputs(_feat(), 0, 5)
