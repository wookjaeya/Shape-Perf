"""Build model inputs for a (feature, padded length) pair in the model's input order."""
import numpy as np

from . import squad


def inputs_for(model, feat, feature_index, length, batch=1):
    """List of numpy arrays in models.json input order + name->array dict."""
    if batch != 1:
        raise NotImplementedError("batch=1 only (spec §1 scope)")
    x = squad.make_padded_inputs(feat, feature_index, length)
    arrs = [np.ascontiguousarray(x[i["feature_key"]]) for i in model["inputs"]]
    for a, i in zip(arrs, model["inputs"]):
        assert a.dtype == np.dtype(i["dtype"]), (i["name"], a.dtype)
    return arrs, {i["name"]: a for a, i in zip(arrs, model["inputs"])}


def outputs_by_name(model, output_list, output_names_in_order):
    return dict(zip(output_names_in_order, output_list))
