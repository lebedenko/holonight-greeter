#!/usr/bin/env python3
"""Run tests with disposable user paths and unavailable desktop service endpoints."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="greeter-tests-") as directory:
    root = Path(directory)
    env = dict(PATH=os.defpath, LANG="C.UTF-8", HOME=str(root),
               QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software",
               QT_QPA_PLATFORMTHEME="", QT_FORCE_STDERR_LOGGING="1",
               DBUS_SESSION_BUS_ADDRESS=f"unix:path={root}/no-session-bus",
               DBUS_SYSTEM_BUS_ADDRESS=f"unix:path={root}/no-system-bus",
               GREETD_SOCK=str(root / "no-greetd"))
    for name in ("QT_QUICK_CONTROLS_STYLE", "QT_SCALE_FACTOR", "LD_LIBRARY_PATH"):
        if name in os.environ:
            env[name] = os.environ[name]
    for name in ("XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME", "XDG_RUNTIME_DIR",
                 "XDG_CONFIG_DIRS", "XDG_DATA_DIRS", "XDG_STATE_HOME"):
        path = root / name
        path.mkdir(mode=0o700)
        env[name] = str(path)
    result = subprocess.run(sys.argv[1:], env=env, timeout=110)
    sys.exit(result.returncode)
