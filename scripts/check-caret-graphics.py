#!/usr/bin/env python3
"""Run the production caret fixture on isolated headless Sway with OpenGL."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('prefix', type=Path)
    parser.add_argument('--logs', type=Path, required=True)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--exact-geometry', action='store_true',
                        help='Run the retained UQC-217 failing reproduction')
    mode.add_argument('--layer-candidate', action='store_true',
                      help='Run the rejected UQC-218 experiment; not repair acceptance')
    args = parser.parse_args()
    args.logs.mkdir(parents=True, exist_ok=True)
    prefix = args.prefix.resolve()
    runner = Path(__file__).with_name('run-isolated-test.py')
    with tempfile.TemporaryDirectory(prefix='greeter-caret-') as directory:
        root = Path(directory)
        config = root / 'sway.conf'
        # Keep compositor tiling from asynchronously replacing test dimensions.
        config.write_text('output HEADLESS-1 mode 2560x1600 scale 1\n'
                          'for_window [app_id=".*"] floating enable\n')
        env = dict(PATH=os.defpath, LANG='C.UTF-8', HOME=str(root),
                   XDG_RUNTIME_DIR=str(root), WLR_BACKENDS='headless',
                   WLR_RENDERER='pixman', WLR_LIBINPUT_NO_DEVICES='1')
        with (args.logs / 'sway.log').open('w') as log:
            compositor = subprocess.Popen(['sway', '--config', str(config)],
                                          env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 10
                sockets = []
                while time.monotonic() < deadline:
                    sockets = [p for p in root.glob('wayland-*') if p.is_socket()]
                    if sockets:
                        break
                    if compositor.poll() is not None:
                        raise RuntimeError('Headless Sway exited; see sway.log')
                    time.sleep(.05)
                if not sockets:
                    raise RuntimeError('No isolated Wayland socket')
                env.update(WAYLAND_DISPLAY=str(sockets[0]), QT_QPA_PLATFORM='wayland',
                           GREETER_CARET_GRAPHICS='1',
                           QT_PLUGIN_PATH=str(prefix / 'lib/qt6/plugins'),
                           LD_LIBRARY_PATH=str(prefix / 'lib'))
                failures = []
                for style in ('Holonight', 'Fusion'):
                    for scale in ('1', '1.25'):
                        case = f'{style}-{scale}'
                        with (args.logs / (case + '.log')).open('w') as output:
                            result = subprocess.run(
                                ['python3', str(runner), str(args.binary.resolve()),
                                 '--gtest_filter=RuntimeControls.' +
                                 ('DISABLED_PasswordCaretLayerCandidate' if args.layer_candidate else
                                  'DISABLED_PasswordCaretRecordedGeometry'
                                  if args.exact_geometry else 'PasswordCaretPixels'),
                                 '--gtest_also_run_disabled_tests'],
                                env=dict(env, QT_QUICK_CONTROLS_STYLE=style,
                                         QT_SCALE_FACTOR=scale,
                                         GREETER_CARET_ARTIFACTS=str((args.logs / case).resolve())), stdout=output,
                                stderr=subprocess.STDOUT, timeout=120)
                        print(case, result.returncode, flush=True)
                        if result.returncode:
                            failures.append(case)
                if failures:
                    raise RuntimeError('Caret comparison failed: ' + ', '.join(failures))
            finally:
                compositor.terminate()
                try:
                    compositor.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    compositor.kill()
                    compositor.wait()


if __name__ == '__main__':
    main()
