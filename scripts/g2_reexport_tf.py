#!/usr/bin/env python3
"""Spec §4.4 step 2: re-export model A's *weights* with a variable sequence axis
through the original pipeline (Google BERT modeling.py + run_squad.create_model
-> TensorFlow graph -> tf2onnx), because the official artifact carries
length-256 constants (results/g2/original_artifact_shape_check.json).

Runs ONLY in the export environment (env/requirements-export.in):
    $SHAPEPERF_WORK/venv-export/bin/python scripts/g2_reexport_tf.py

What is taken from where
  weights        : every TF variable is filled from the bertsquad-12.onnx
                   initializer of the same name (names are TF variable names).
                   Missing or shape-mismatched weights abort the export.
  model code     : pinned google-research/bert modeling.py; create_model() is
                   extracted verbatim from run_squad.py (the module itself is
                   not importable under TF2 because of tf.flags/tf.contrib.tpu).
  TF2 port       : two textual substitutions in modeling.py, recorded as a diff:
                   `import tensorflow as tf` -> `tensorflow.compat.v1`, and
                   tf.contrib.layers.layer_norm -> tf_slim.layers.layer_norm
                   (tf_slim is the extracted tf.contrib.layers code).
  I/O signature  : same tensor names/dtypes as the official artifact, sequence
                   dimension left symbolic.
The result must still pass scripts/g2_validate_reexport.py before use, and
papers must call it "a re-export of the ONNX Model Zoo bertsquad-12 weights",
not the Model Zoo artifact (spec §4.4).
"""
import argparse
import ast
import difflib
import hashlib
import json
import sys
import time
import types
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
BERT_DIR = REPO_ROOT / "third_party/google_bert"


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def load_modeling():
    src = (BERT_DIR / "modeling.py").read_text()
    subs = [("import tensorflow as tf\n", "import tensorflow.compat.v1 as tf\nimport tf_slim\n", 1),
            ("tf.contrib.layers.layer_norm(", "tf_slim.layers.layer_norm(", 1)]
    patched = src
    for old, new, count in subs:
        assert patched.count(old) == count, f"expected {count}x {old!r}"
        patched = patched.replace(old, new)
    diff = "".join(difflib.unified_diff(src.splitlines(True), patched.splitlines(True),
                                        "modeling.py", "modeling.py (TF2 compat port)"))
    mod = types.ModuleType("modeling")
    exec(compile(patched, "modeling.py", "exec"), mod.__dict__)
    sys.modules["modeling"] = mod
    return mod, diff


def load_create_model(modeling, tf):
    src = (BERT_DIR / "run_squad.py").read_text()
    tree = ast.parse(src)
    fn = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "create_model")
    code = ast.get_source_segment(src, fn)
    ns = {"modeling": modeling, "tf": tf}
    exec(compile(code, "run_squad.py:create_model", "exec"), ns)
    return ns["create_model"], code


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default=str(REPO_ROOT / "data/artifacts/bertsquad-12.onnx"))
    ap.add_argument("--out", default=str(REPO_ROOT / "data/artifacts/derived/bertsquad-12-reexport-dynseq.onnx"))
    ap.add_argument("--opset", type=int, default=12, help="opset of the official artifact")
    args = ap.parse_args()

    import numpy as np
    import onnx
    from onnx import numpy_helper
    import tensorflow.compat.v1 as tf
    import tf2onnx
    tf.disable_v2_behavior()

    src_model = onnx.load(args.src)
    weights = {i.name: numpy_helper.to_array(i) for i in src_model.graph.initializer}

    def const_scalar(name):
        return int(np.asarray(weights[name]).reshape(-1)[0])

    n_layers = len({k.split("/")[2] for k in weights if k.startswith("bert/encoder/layer_")})
    # num heads / head size from the attention Reshape shape constants of the artifact.
    heads = const_scalar("bert/encoder/layer_0/attention/self/Reshape/shape/2:0")
    head_size = const_scalar("bert/encoder/layer_0/attention/self/Reshape/shape/3:0")
    hidden = weights["bert/embeddings/word_embeddings:0"].shape[1]
    assert heads * head_size == hidden, (heads, head_size, hidden)
    op_hist = {}
    for n in src_model.graph.node:
        op_hist[n.op_type] = op_hist.get(n.op_type, 0) + 1
    cfg_dict = {
        "vocab_size": weights["bert/embeddings/word_embeddings:0"].shape[0],
        "hidden_size": hidden,
        "num_hidden_layers": n_layers,
        "num_attention_heads": heads,
        "intermediate_size": weights["bert/encoder/layer_0/intermediate/dense/kernel:0"].shape[1],
        "hidden_act": "gelu",  # modeling.gelu is the tanh approximation; artifact has Tanh x n_layers
        "hidden_dropout_prob": 0.1, "attention_probs_dropout_prob": 0.1,  # unused, is_training=False
        "max_position_embeddings": weights["bert/embeddings/position_embeddings:0"].shape[0],
        "type_vocab_size": weights["bert/embeddings/token_type_embeddings:0"].shape[0],
        "initializer_range": 0.02,
    }
    assert op_hist.get("Tanh") == n_layers, op_hist.get("Tanh")
    cfg_dict = {k: (int(v) if isinstance(v, (np.integer,)) else v) for k, v in cfg_dict.items()}

    modeling, patch_diff = load_modeling()
    create_model, create_model_src = load_create_model(modeling, tf)
    config = modeling.BertConfig.from_dict(cfg_dict)

    g = tf.Graph()
    with g.as_default():
        unique_ids = tf.placeholder(tf.int64, [None], name="unique_ids_raw_output___9")
        input_ids = tf.placeholder(tf.int64, [None, None], name="input_ids")
        input_mask = tf.placeholder(tf.int64, [None, None], name="input_mask")
        segment_ids = tf.placeholder(tf.int64, [None, None], name="segment_ids")
        start_logits, end_logits = create_model(config, False, input_ids, input_mask, segment_ids,
                                                use_one_hot_embeddings=False)
        tf.identity(unique_ids, name="unique_ids")
        assert start_logits.name == "unstack:0" and end_logits.name == "unstack:1", \
            (start_logits.name, end_logits.name)
        tvars = tf.global_variables()
        loaded, unused_tf, assigns = [], [], []
        for v in tvars:
            if v.name in weights:
                w = weights[v.name]
                assert tuple(w.shape) == tuple(v.shape.as_list()), (v.name, w.shape, v.shape)
                assigns.append(tf.assign(v, w.astype(v.dtype.as_numpy_dtype)))
                loaded.append(v.name)
            else:
                unused_tf.append(v.name)
        with tf.Session(graph=g) as sess:
            sess.run(tf.global_variables_initializer())
            sess.run(assigns)
            frozen = tf.graph_util.convert_variables_to_constants(
                sess, g.as_graph_def(), ["unstack", "unique_ids"])
    # Any variable the frozen outputs depend on must have come from the artifact.
    frozen_consts = {n.name + ":0" for n in frozen.node if n.op == "Const"}
    unused_needed = [v for v in unused_tf if v in frozen_consts]
    assert not unused_needed, f"outputs depend on weights missing from the artifact: {unused_needed}"
    unused_onnx = sorted(k for k in weights if weights[k].size >= 64 and k not in loaded)

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    t0 = time.time()
    model_proto, _ = tf2onnx.convert.from_graph_def(
        frozen,
        input_names=["unique_ids_raw_output___9:0", "segment_ids:0", "input_mask:0", "input_ids:0"],
        output_names=["unstack:1", "unstack:0", "unique_ids:0"],
        opset=args.opset, output_path=str(out))
    meta = {
        "source_artifact": args.src, "source_sha256": sha256(args.src),
        "output": str(out), "output_sha256": sha256(out),
        "exporter": f"tf2onnx {tf2onnx.__version__}", "tensorflow": tf.__version__,
        "opset": args.opset, "export_seconds": round(time.time() - t0, 1),
        "bert_config_derived_from_artifact": cfg_dict,
        "google_bert_commit": "eedf5716ce1268e56f0a50264a88cafad334ac61",
        "modeling_py_sha256": sha256(BERT_DIR / "modeling.py"),
        "run_squad_py_sha256": sha256(BERT_DIR / "run_squad.py"),
        "modeling_py_tf2_port_diff": patch_diff,
        "create_model_source": create_model_src,
        "n_tf_variables": len(tvars), "n_loaded_from_artifact": len(loaded),
        "tf_variables_not_in_artifact_and_pruned": unused_tf,
        "artifact_weights_not_used": unused_onnx,
        "label": "re-export of the ONNX Model Zoo bertsquad-12 weights (NOT the Model Zoo artifact)",
    }
    with open(str(out) + ".json", "w") as f:
        json.dump(meta, f, indent=2)
    # tracked copy (the model itself is too large for git; its SHA-256 identifies it)
    tracked = REPO_ROOT / "results/g2" / (out.name + ".export_meta.json")
    tracked.parent.mkdir(parents=True, exist_ok=True)
    with open(tracked, "w") as f:
        json.dump(meta, f, indent=2)
    print(json.dumps({k: meta[k] for k in ["output_sha256", "exporter", "tensorflow", "n_loaded_from_artifact",
                                           "tf_variables_not_in_artifact_and_pruned", "artifact_weights_not_used"]},
                     indent=1))


if __name__ == "__main__":
    main()
