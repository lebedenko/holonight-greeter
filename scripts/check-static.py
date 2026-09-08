#!/usr/bin/env python3
"""Check generated metadata and analyze owned C++ after building all targets."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
for name in ("Holonight/Greeter/holonight-greeter.qmltypes",
             "runtime-qml/greeter_runtime_tests.qmltypes"):
    assert "Module {" in (build / name).read_text(), name
entries = json.loads((build / "compile_commands.json").read_text())
# clang does not accept this GCC-only warning option on Arch.
for entry in entries:
    entry["command"] = entry["command"].replace("-Wno-template-id-cdtor", "").replace("-mno-direct-extern-access", "")
with tempfile.TemporaryDirectory(prefix="greeter-tidy-") as directory:
    (Path(directory) / "compile_commands.json").write_text(json.dumps(entries))
    sources = sorted({e["file"] for e in entries if Path(e["file"]).is_relative_to(root / "src")
                      or Path(e["file"]).is_relative_to(root / "tests")})
    subprocess.run(["run-clang-tidy", "-p", directory, "-j", "4", *sources], check=True)
