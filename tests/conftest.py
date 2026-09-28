import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
# selectors outside the registry (tests/malicious_*.py) are loaded by class_path,
# which the selector worker allows only when this is set
os.environ.setdefault("SHAPEPERF_ALLOW_CLASS_PATH", "1")
