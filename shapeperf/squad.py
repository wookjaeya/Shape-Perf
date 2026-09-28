"""SQuAD v1.1 preprocessing/postprocessing via the *official* model-A code (R7).

We import read_squad_examples / convert_examples_to_features / write_predictions
from the pinned run_onnx_squad.py and never re-implement them.

Two documented adaptations (see docs/STATUS.md, G1):
  1. tokenization.py imports TensorFlow only for tf.gfile.GFile(vocab, "r").
     When TensorFlow is not installed we register a minimal stand-in module
     whose gfile.GFile is open(..., encoding="utf-8"). Tokenizer logic is
     untouched.
  2. run_onnx_squad.main() indexes outputs as [1, batch, L]; bertsquad-12
     returns [batch, L] and has a 4th input (unique_ids_raw_output___9:0).
     We therefore follow the feed/output handling of the model's official
     notebook (BERT-Squad.ipynb): unique id in, start=unstack:0[0],
     end=unstack:1[0], per *feature* (not per example).
"""
import importlib
import json
import subprocess
import sys
import types
from pathlib import Path

import numpy as np

from .util import THIRD_PARTY_DIR

BERT_SQUAD_DIR = THIRD_PARTY_DIR / "bert_squad"
SQUAD_EVAL = THIRD_PARTY_DIR / "squad" / "evaluate_v1.1.py"

# Official model-A preprocessing parameters (R6 README / R7 defaults).
MODEL_A_PARAMS = dict(max_seq_length=256, doc_stride=128, max_query_length=64,
                      n_best_size=20, max_answer_length=30, do_lower_case=True)

INPUT_NAMES_A = {
    "unique_ids": "unique_ids_raw_output___9:0",
    "input_ids": "input_ids:0",
    "input_mask": "input_mask:0",
    "segment_ids": "segment_ids:0",
}
OUTPUT_NAMES_A = {"start": "unstack:0", "end": "unstack:1", "unique_ids": "unique_ids:0"}

TF_SHIM_NOTE = ("tensorflow not installed: tokenization.py's tf.gfile.GFile replaced by "
                "open(encoding='utf-8'); tokenizer logic unchanged")


def _install_tf_shim():
    try:
        importlib.import_module("tensorflow")
        return None
    except ImportError:
        pass
    tf = types.ModuleType("tensorflow")
    tf.gfile = types.SimpleNamespace(GFile=lambda p, mode="r": open(p, mode, encoding="utf-8"))
    tf.__shapeperf_shim__ = True
    sys.modules["tensorflow"] = tf
    return TF_SHIM_NOTE


def load_official():
    """Return (run_onnx_squad module, tokenization module, shim note or None)."""
    if not (BERT_SQUAD_DIR / "run_onnx_squad.py").exists():
        raise FileNotFoundError("run scripts/fetch_artifacts.py first")
    note = _install_tf_shim()
    if str(BERT_SQUAD_DIR) not in sys.path:
        sys.path.insert(0, str(BERT_SQUAD_DIR))
    tokenization = importlib.import_module("tokenization")
    ros = importlib.import_module("run_onnx_squad")
    return ros, tokenization, note


def valid_length(mask_row):
    """L(x): number of positions marked valid in input_mask (spec §4.2)."""
    return int(np.asarray(mask_row).sum())


def check_trailing_padding(input_ids, input_mask, segment_ids):
    """Verify the spec §4.2 precondition: valid tokens first, then mask=0 padding
    with id 0 and segment 0. Returns list of offending feature indices."""
    bad = []
    for i in range(input_mask.shape[0]):
        L = valid_length(input_mask[i])
        m = input_mask[i]
        if not (np.all(m[:L] == 1) and np.all(m[L:] == 0)
                and np.all(input_ids[i, L:] == 0) and np.all(segment_ids[i, L:] == 0)):
            bad.append(i)
    return bad


def pad_to(row, s, pad_value=0):
    """Take a feature row padded to the official length, keep its valid prefix
    and re-pad (or truncate trailing padding) to length s. Never touches valid
    tokens (spec §4.3: no re-tokenisation, no new windows)."""
    row = np.asarray(row)
    return _pad_or_trim(row, s, pad_value)


def _pad_or_trim(row, s, pad_value):
    if s <= row.shape[-1]:
        return row[..., :s].copy()
    out = np.full(row.shape[:-1] + (s,), pad_value, dtype=row.dtype)
    out[..., :row.shape[-1]] = row
    return out


def make_padded_inputs(feat, idx, s):
    """Inputs of feature idx at padded length s (batch 1). Raises if s < L(x)."""
    L = int(feat["valid_length"][idx])
    if s < L:
        raise ValueError(f"padded length {s} < valid length {L}")
    return {
        "unique_ids": np.array([int(feat["unique_id"][idx])], dtype=np.int64),
        "input_ids": pad_to(feat["input_ids"][idx:idx + 1], s),
        "input_mask": pad_to(feat["input_mask"][idx:idx + 1], s),
        "segment_ids": pad_to(feat["segment_ids"][idx:idx + 1], s),
    }


def load_features(npz_path):
    d = np.load(npz_path, allow_pickle=False)
    return {k: d[k] for k in d.files}


def run_official_eval(dataset_json, predictions_json):
    """Run the pinned official evaluate_v1.1.py as a subprocess; returns dict."""
    out = subprocess.run([sys.executable, str(SQUAD_EVAL), str(dataset_json), str(predictions_json)],
                         capture_output=True, text=True, check=True).stdout
    return json.loads(out.strip().splitlines()[-1])


def write_official_predictions(ros, examples, extra, start_logits, end_logits, out_dir, lengths=None):
    """Call the official write_predictions with per-feature logits.

    lengths[i] (optional) is the padded length the feature was run at; only the
    first lengths[i] logits are passed on, exactly as a runtime at that length
    would return them."""
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    n = [start_logits.shape[1]] * len(extra) if lengths is None else [int(x) for x in lengths]
    results = [ros.RawResult(unique_id=int(f.unique_id),
                             start_logits=[float(x) for x in start_logits[i][:n[i]]],
                             end_logits=[float(x) for x in end_logits[i][:n[i]]])
               for i, f in enumerate(extra)]
    p = MODEL_A_PARAMS
    pred = out_dir / "predictions.json"
    ros.write_predictions(examples, extra, results, p["n_best_size"], p["max_answer_length"],
                          p["do_lower_case"], str(pred), str(out_dir / "nbest_predictions.json"))
    return pred
