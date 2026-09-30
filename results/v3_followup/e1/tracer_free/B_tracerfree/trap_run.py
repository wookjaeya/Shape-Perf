import sys
sys.path.insert(0, "/home/user/work/onnx-mlir/build/Release/lib")
import numpy as np
from PyRuntime import OMExecutionSession
first, second, which = sys.argv[1], sys.argv[2], int(sys.argv[3])
s = [OMExecutionSession(shared_lib_path=first), OMExecutionSession(shared_lib_path=second)]
x = np.load("/home/user/work/v3/g2/K/L0064/input.npy"); e = np.load("/home/user/work/v3/g2/K/L0064/expected.npy")
out = s[which].run([x])
print("EXACT" if (len(out) == 1 and out[0].dtype == e.dtype and np.array_equal(out[0], e)) else "MISMATCH")
