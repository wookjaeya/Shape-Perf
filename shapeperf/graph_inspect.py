"""ONNX graph inspection for shape-change legitimacy (spec §4.4 step 1, G1/G2)."""
import collections

import numpy as np
import onnx
from onnx import numpy_helper

SHAPE_CONSUMERS = {"Reshape", "Expand", "Tile", "ConstantOfShape", "Slice", "Unsqueeze",
                   "Concat", "Range", "Mul", "Resize", "Pad"}


def _dims(v):
    return [d.dim_value if d.HasField("dim_value") else (d.dim_param or None)
            for d in v.type.tensor_type.shape.dim]


def io_signature(model):
    g = model.graph
    init = {i.name for i in g.initializer}
    ins = [{"name": v.name, "elem_type": onnx.TensorProto.DataType.Name(v.type.tensor_type.elem_type),
            "dims": _dims(v)} for v in g.input if v.name not in init]
    outs = [{"name": v.name, "elem_type": onnx.TensorProto.DataType.Name(v.type.tensor_type.elem_type),
             "dims": _dims(v)} for v in g.output]
    return ins, outs


def small_int_constants(model, max_size=16):
    g = model.graph
    consts = {}
    for i in g.initializer:
        a = numpy_helper.to_array(i)
        if a.size <= max_size and a.dtype.kind in "iu":
            consts[i.name] = a
    for n in g.node:
        if n.op_type == "Constant":
            for at in n.attribute:
                if at.name == "value":
                    a = numpy_helper.to_array(at.t)
                    if a.size <= max_size and a.dtype.kind in "iu":
                        consts[n.output[0]] = a
    return consts


def length_constant_report(model, length):
    """Small integer constants that contain `length` and the ops consuming them.
    A hit does not prove a length dependence (256 could be a coincidence), so the
    report is evidence for a human decision plus the runtime test below."""
    consts = small_int_constants(model)
    users = collections.defaultdict(list)
    for n in model.graph.node:
        for i in n.input:
            users[i].append({"op": n.op_type, "node": n.name})
    hits = []
    for k, v in consts.items():
        if length in v.flatten().tolist():
            hits.append({"const": k, "value": v.tolist(), "consumers": users.get(k, [])})
    by_op = collections.Counter(u["op"] for h in hits for u in h["consumers"])
    return {"length": length, "n_constants_containing_length": len(hits),
            "consumer_op_counts": dict(by_op), "constants": hits}


def op_histogram(model):
    return dict(collections.Counter(n.op_type for n in model.graph.node).most_common())


def with_input_length(model, length, axis=1):
    """Copy of `model` with ONLY the declared input/output dims on `axis` changed.
    Used to *test* whether metadata-only changes are enough (spec §4.4 step 1);
    this copy is never used as an experimental artifact."""
    m = onnx.ModelProto()
    m.CopyFrom(model)
    for v in list(m.graph.input) + list(m.graph.output):
        dims = v.type.tensor_type.shape.dim
        if len(dims) > axis:
            dims[axis].Clear()
            dims[axis].dim_value = length
    return m


def weight_summary(model, min_size=64):
    return {i.name: list(i.dims) for i in model.graph.initializer
            if int(np.prod(i.dims)) >= min_size}


def semantic_fingerprint(model):
    """Order- and name-independent fingerprint: multiset of (op_type, domain,
    attributes) and multiset of initializer (dtype, shape, content hash), plus
    the I/O signature. Two exports with equal fingerprints differ at most in
    node order/names and wiring that this does not capture; functional
    equivalence is established separately by output comparison."""
    import hashlib
    import json as _json

    def attr_val(a):
        v = onnx.helper.get_attribute_value(a)
        if isinstance(v, onnx.TensorProto):
            return hashlib.sha256(numpy_helper.to_array(v).tobytes()).hexdigest()[:16]
        if isinstance(v, bytes):
            return v.decode(errors="replace")
        if isinstance(v, (list, tuple)):
            return [x.decode(errors="replace") if isinstance(x, bytes) else x for x in v]
        return v if isinstance(v, (int, float, str)) else str(v)

    nodes = sorted(_json.dumps([n.op_type, n.domain, sorted((a.name, attr_val(a)) for a in n.attribute)],
                               default=str) for n in model.graph.node)
    inits = sorted(_json.dumps([i.data_type, list(i.dims),
                                hashlib.sha256(numpy_helper.to_array(i).tobytes()).hexdigest()[:16]])
                   for i in model.graph.initializer)
    ins, outs = io_signature(model)
    io = [(x["name"], x["elem_type"], len(x["dims"])) for x in ins + outs]
    h = hashlib.sha256(_json.dumps([nodes, inits, io]).encode()).hexdigest()
    return {"sha256": h, "n_nodes": len(nodes), "n_initializers": len(inits)}
