#!/usr/bin/env python3
"""Check product and acceptance QML metadata after building all targets."""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
for name in ("Holonight/Greeter/holonight-greeter.qmltypes",
             "runtime-qml/greeter_runtime_tests.qmltypes"):
    assert "Module {" in (build / name).read_text(), name
