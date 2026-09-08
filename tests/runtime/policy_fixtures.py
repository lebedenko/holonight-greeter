#!/usr/bin/env python3
"""Each forbidden import/use must fail independently; valid fixtures must pass."""
from pathlib import Path
import subprocess
import tempfile

checker = Path(__file__).resolve().parents[2] / "scripts/check-runtime-qml-imports.sh"
fixtures = {
    "runtime": (True, "import QtQuick.Controls as Controls\nControls.Button {}"),
    "core": (True, "import Holonight.Core\nHnLabel {}"),
    "composite": (True, "import Holonight.Controls\nHnIconComboBox {}"),
    "direct": (False, "import Holonight as Hn\nHn.Button {}"),
    "basic": (False, "import QtQuick.Controls.Basic as Basic\nBasic.Button {}"),
    "fusion": (False, "import QtQuick.Controls.Fusion as Fusion\nFusion.Button {}"),
    "unnamespaced": (False, "import QtQuick.Controls\nButton {}"),
    "alias": (False, "import QtQuick.Controls as C\nC.Button {}"),
    "missing-import": (False, "Controls.Button {}"),
    "bare-instance": (False, "import QtQuick.Controls as Controls\nButton {}"),
    "bare-enum": (False, "import QtQuick.Controls as Controls\nItem { property int p: Popup.Item }"),
    "bare-attached": (False, "import QtQuick.Controls as Controls\nItem { ToolTip.text: 'help' }"),
    "core-import": (False, "Item { property var p: HnMetrics.controlHeight(0) }"),
    "composite-import": (False, "HnIconComboBox {}"),
    "internal-enum": (False, "Item { property int p: HnSelectableDelegate.Outline }"),
}
for name, (valid, source) in fixtures.items():
    with tempfile.TemporaryDirectory(prefix="greeter-policy-") as directory:
        root = Path(directory)
        (root / "qml").mkdir()
        (root / "qml/Fixture.qml").write_text(source + "\n")
        result = subprocess.run(["bash", str(checker), str(root)], capture_output=True, text=True)
        assert (result.returncode == 0) == valid, f"{name}: {result.stdout}{result.stderr}"
        print(f"PASS {name}")
