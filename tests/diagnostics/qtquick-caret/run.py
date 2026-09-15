#!/usr/bin/env python3
"""Run the standalone QtQuick diagnostic with disposable paths and no theme."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('binary', type=Path)
parser.add_argument('--graphics', action='store_true')
parser.add_argument('--logs', type=Path, required=True)
args = parser.parse_args()
args.logs.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='plain-qtquick-caret-') as directory:
    root = Path(directory)
    env = dict(PATH=os.defpath, LANG='C.UTF-8', HOME=str(root),
               XDG_RUNTIME_DIR=str(root), XDG_CONFIG_HOME=str(root),
               QT_QPA_PLATFORMTHEME='', QT_FORCE_STDERR_LOGGING='1',
               QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software')
    compositor = None
    try:
        if args.graphics:
            config = root / 'sway.conf'
            config.write_text('output HEADLESS-1 mode 2560x1600 scale 1\n'
                              'for_window [app_id=".*"] floating enable\n')
            with (args.logs / 'sway.log').open('w') as log:
                compositor = subprocess.Popen(['sway', '--config', str(config)],
                    env=dict(env, WLR_BACKENDS='headless', WLR_RENDERER='pixman',
                             WLR_LIBINPUT_NO_DEVICES='1'),
                    stdout=log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 10
            while True:
                sockets = [p for p in root.glob('wayland-*') if p.is_socket()]
                if sockets:
                    break
                if compositor.poll() is not None or time.monotonic() >= deadline:
                    raise RuntimeError('Private compositor unavailable; see sway.log')
                time.sleep(.05)
            env.update(WAYLAND_DISPLAY=str(sockets[0]), QT_QPA_PLATFORM='wayland',
                       QT_QUICK_BACKEND='rhi', QSG_RHI_BACKEND='opengl', QSG_INFO='1')
        results = {}
        for scale in ('1', '1.25'):
            with (args.logs / (scale + '.log')).open('w') as log:
                result = subprocess.run([str(args.binary.resolve())],
                    env=dict(env, QT_SCALE_FACTOR=scale,
                             GREETER_CARET_ARTIFACTS=str((args.logs / scale).resolve())),
                    stdout=log, stderr=subprocess.STDOUT, timeout=30)
            results[scale] = result.returncode
            print(scale, result.returncode, flush=True)
        (args.logs / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
        raise SystemExit(1 if any(results.values()) else 0)
    finally:
        if compositor:
            compositor.terminate()
            try:
                compositor.wait(timeout=5)
            except subprocess.TimeoutExpired:
                compositor.kill()
                compositor.wait()
