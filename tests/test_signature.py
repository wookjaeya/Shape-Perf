"""opt-report parsing and normalization (spec §6.2): trip counts, SSA names and
shape literals must not change the structural signature; decisions must."""
from shapeperf import signature as S

REPORT_128 = """
some compiler chatter
==SIMD-REPORT==, onnx.Add-simd, bert/encoder/layer_0/add, simd, 4, 98304
==SIMD-REPORT==, onnx.MatMul-simd, bert/encoder/layer_0/q/MatMul, simd matmul, 4, 128
==SIMD-REPORT==, onnx.Softmax, bert/encoder/layer_0/Softmax, no simd because short loop of 3 elements, 0, 0
==PAR-REPORT==, onnx.Add-par, bert/encoder/layer_0/add, outermost loop, 0, 128
"""
REPORT_129 = REPORT_128.replace("98304", "99072").replace(", 128\n", ", 129\n")
REPORT_DECISION_CHANGE = REPORT_128.replace("onnx.Add-simd, bert/encoder/layer_0/add, simd, 4",
                                            "onnx.Add, bert/encoder/layer_0/add, unsupported, 0")


def test_parse_fields():
    recs = S.parse_opt_report(REPORT_128)
    assert len(recs) == 4
    add = recs[0]
    assert add == {"kind": "SIMD", "op": "onnx.Add", "applied": True, "node": "bert/encoder/layer_0/add",
                   "message": "simd", "value": 4, "trip_count": 98304}
    assert recs[2]["applied"] is False and recs[2]["value"] == 0
    assert recs[3]["kind"] == "PAR" and recs[3]["applied"] is True


def test_trip_counts_do_not_change_signature():
    a = S.report_signature(S.parse_opt_report(REPORT_128))["hash"]
    b = S.report_signature(S.parse_opt_report(REPORT_129))["hash"]
    assert a == b


def test_decision_changes_signature():
    a = S.report_signature(S.parse_opt_report(REPORT_128))["hash"]
    c = S.report_signature(S.parse_opt_report(REPORT_DECISION_CHANGE))["hash"]
    assert a != c


def test_message_integers_normalized():
    r3 = S.parse_opt_report(REPORT_128)
    r5 = S.parse_opt_report(REPORT_128.replace("loop of 3 elements", "loop of 5 elements"))
    assert S.report_signature(r3)["hash"] == S.report_signature(r5)["hash"]


IR_A = """#map = affine_map<(d0) -> (d0 * 128)>
module {
  func.func @main_graph(%arg0: memref<1x128xi64>) -> memref<1x128xf32> {
    %0 = memref.alloc() {alignment = 16 : i64} : memref<1x128xf32> loc(#loc1)
    affine.for %arg1 = 0 to 128 step 4 {
      %1 = vector.load %0[%c0, %arg1] : memref<1x128xf32>, vector<4xf32>
      affine.for %arg2 = 0 to 3 {
      }
    }
    return %0 : memref<1x128xf32>
  }
}
#loc1 = loc("x")
"""
IR_B = IR_A.replace("128", "129").replace("%1 =", "%77 =")
IR_C = IR_A.replace("vector<4xf32>", "vector<8xf32>")


def test_ir_structure_ignores_literals_and_ssa():
    assert S.ir_structure(IR_A)["hash"] == S.ir_structure(IR_B)["hash"]
    assert S.raw_ir_hash(IR_A) != S.raw_ir_hash(IR_B)          # ablation §11.2-3 differs


def test_ir_structure_sees_vector_width_and_loops():
    assert S.ir_structure(IR_A)["hash"] != S.ir_structure(IR_C)["hash"]
    f = S.ir_structure(IR_A)["functions"]["main_graph"]
    assert f["loops"] == {"affine.for@2": 1, "affine.for@3": 1}
    assert f["ops"]["vector.load"] == 1


def test_entry_signature():
    assert S.entry_signature_from_ir(IR_A) == ["memref<1x128xi64>"]


def test_matmul_path_classification():
    recs = S.parse_opt_report(REPORT_128)
    assert S.matmul_path(["malloc", "free"], recs)[0] == "compiler-generated"
    assert S.matmul_path(["cblas_sgemm"], recs)[0] == "external-library"
    assert S.matmul_path(None, recs)[0] == "unknown"


def test_sig_v2_canonicalizes_compiler_counters():
    known = {"enc/layer_0/mul_3", "enc/layer_0/q/MatMul", "enc/Reshape_1", "enc/Rsqrt__518"}
    assert S.canonical_node("enc/layer_0/mul_3_12", known) == "enc/layer_0/mul_3"
    assert S.canonical_node("enc/layer_0/mul_3", known) == "enc/layer_0/mul_3"
    assert S.canonical_node("enc/Rsqrt__518", known) == "enc/Rsqrt__518"          # original name kept
    assert S.canonical_node("enc/layer_0/q/MatMul-enc/Reshape_1_80", known) == "enc/layer_0/q/MatMul-enc/Reshape_1"
    a = "==SIMD-REPORT==, onnx.Gelu-simd, enc/layer_0/mul_3_12, unary fully flattened, 16, 98304\n"
    b = a.replace("mul_3_12", "mul_3_13")
    ha = S.report_signature(S.parse_opt_report(a), known)
    hb = S.report_signature(S.parse_opt_report(b), known)
    assert ha["hash"] == hb["hash"] and ha["version"] == "sig-v2"
    assert S.report_signature(S.parse_opt_report(a))["hash"] != S.report_signature(S.parse_opt_report(b))["hash"]


def test_corrupt_report_is_flagged():
    text = REPORT_128 + "==SIMD-REPORT==, onnx.Sqrt-simd, x/Rsqrt, unary f\n"
    sig = S.report_signature(S.parse_opt_report(text))
    assert sig["integrity"] == "corrupt" and sig["n_unparsed"] == 1
