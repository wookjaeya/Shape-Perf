#!/usr/bin/env python3
"""Official SQuAD preprocessing, question/window tracking and shape catalog
(spec §4.1, §4.2, §4.3, §12).

Outputs (under --out, default data/features/A):
  features.npz        input_ids/input_mask/segment_ids at the official length,
                      unique_id, example_index, window_index, valid_length, qas_id
  features_extra.pkl  official Feature tuples (tokens, token_to_orig_map, ...) for
                      the official write_predictions
  catalog.json        counts, S_observed with frequencies, padding check,
                      anchor feature for the controlled padding sweep, provenance
"""
import argparse
import collections
import pickle
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from shapeperf import squad  # noqa: E402
from shapeperf.util import REPO_ROOT, git_head, read_json, sha256_file, write_json  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dataset", default=str(REPO_ROOT / "data/artifacts/squad/dev-v1.1.json"))
    ap.add_argument("--vocab", default=str(REPO_ROOT / "data/artifacts/uncased_L-12_H-768_A-12/vocab.txt"))
    ap.add_argument("--out", default=str(REPO_ROOT / "data/features/A"))
    ap.add_argument("--limit-examples", type=int, default=None,
                    help="smoke tests only; catalog records the limit")
    args = ap.parse_args()
    p = squad.MODEL_A_PARAMS

    ros, tokenization, shim_note = squad.load_official()
    tokenizer = tokenization.FullTokenizer(vocab_file=args.vocab, do_lower_case=p["do_lower_case"])
    examples = ros.read_squad_examples(input_file=args.dataset)
    if args.limit_examples:
        examples = examples[:args.limit_examples]
    input_ids, input_mask, segment_ids, extra = ros.convert_examples_to_features(
        examples, tokenizer, p["max_seq_length"], p["doc_stride"], p["max_query_length"])

    n = len(extra)
    example_index = np.array([f.example_index for f in extra], dtype=np.int64)
    unique_id = np.array([f.unique_id for f in extra], dtype=np.int64)
    # window id = position of the feature among its example's doc spans; the
    # official loop appends spans in doc_span_index order.
    counter = collections.Counter()
    window_index = np.empty(n, dtype=np.int64)
    for i, e in enumerate(example_index):
        window_index[i] = counter[e]
        counter[e] += 1
    qas_id = np.array([examples[e].qas_id for e in example_index])
    vlen = np.array([squad.valid_length(r) for r in input_mask], dtype=np.int64)
    tok_len = np.array([len(f.tokens) for f in extra], dtype=np.int64)

    bad = squad.check_trailing_padding(input_ids, input_mask, segment_ids)
    mismatched_tokens = int(np.sum(vlen != tok_len))

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    np.savez(out / "features.npz", input_ids=input_ids, input_mask=input_mask,
             segment_ids=segment_ids, unique_id=unique_id, example_index=example_index,
             window_index=window_index, valid_length=vlen, qas_id=qas_id)
    with open(out / "features_extra.pkl", "wb") as f:
        pickle.dump({"extra": extra, "examples": examples}, f)

    freq = collections.Counter(vlen.tolist())
    s_observed = sorted(freq)
    # Anchor for the first controlled padding sweep (spec §4.3): shortest valid
    # feature, ties broken by (question id, window id) lexicographic order.
    # Fixed before any performance is observed.
    order = sorted(range(n), key=lambda i: (int(vlen[i]), str(qas_id[i]), int(window_index[i])))
    a = order[0]
    lock = read_json(REPO_ROOT / "artifacts.lock.json")["artifacts"]
    catalog = {
        "model": "A (bertsquad-12)",
        "params": p,
        "limit_examples": args.limit_examples,
        "n_questions": len({str(q) for q in qas_id}),
        "n_examples_read": len(examples),
        "n_features": n,
        "n_unique_valid_lengths": len(s_observed),
        "valid_length_min": int(vlen.min()), "valid_length_max": int(vlen.max()),
        "S_observed": s_observed,
        "S_observed_freq": {str(k): v for k, v in sorted(freq.items())},
        "windows_per_question": dict(collections.Counter(collections.Counter(qas_id.tolist()).values())),
        "trailing_padding_violations": bad[:50],
        "n_trailing_padding_violations": len(bad),
        "valid_length_vs_token_count_mismatches": mismatched_tokens,
        "anchor": {"feature_index": a, "qas_id": str(qas_id[a]), "window_index": int(window_index[a]),
                   "valid_length": int(vlen[a]),
                   "S_padding": [int(vlen[a]), p["max_seq_length"]],
                   "rule": "min valid_length, ties by (qas_id, window_index) lexicographic (spec §4.3)"},
        "note": "S_observed is the distribution of SQuAD preprocessing output, not a datacenter request distribution (spec §4.2).",
        "provenance": {
            "dataset_sha256": sha256_file(args.dataset),
            "vocab_sha256": sha256_file(args.vocab),
            "run_onnx_squad_sha256": lock["run_onnx_squad.py"]["sha256"],
            "tokenization_sha256": lock["tokenization.py"]["sha256"],
            "harness_commit": git_head(),
            "adaptations": [n for n in [shim_note] if n],
        },
    }
    write_json(out / "catalog.json", catalog)
    print({k: catalog[k] for k in ["n_questions", "n_features", "n_unique_valid_lengths",
                                   "valid_length_min", "valid_length_max",
                                   "n_trailing_padding_violations", "anchor"]})


if __name__ == "__main__":
    main()
