#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
build=build/dev
prefix="$PWD/build/deps/prefix"
stage="$PWD/build/installed"
mkdir -p /work/output
    task build PRESET=release
    greetd_identity() {
      if [ -e /etc/greetd/config.toml ]; then sha256sum /etc/greetd/config.toml; else printf 'absent\n'; fi
    }
    greetd_identity > /work/output/greetd-before.txt
    mkdir -p "$stage/usr"
    DESTDIR="$stage" cmake --install build/release
    cp -a "$prefix/." "$stage/usr/"
    test -x "$stage/usr/bin/holonight-greeter"
    test -x "$stage/usr/bin/holonight-greeter-session"
    test -f "$stage/etc/holonight/greeter.toml"
    test -f "$stage/usr/share/holonight-greeter/backgrounds/wallpaper.png"
    test "$(find "$stage/usr/share/pixmaps/faces" -type f | wc -l)" -eq 8
    test -f "$stage/usr/lib/tmpfiles.d/holonight-greeter.conf"
    test -f "$stage/usr/share/doc/holonight-greeter/greetd-config.toml"
    test -f "$stage/usr/share/doc/holonight-greeter/CAGE.md"
    test -x "$stage/usr/libexec/holonight-greeter/holonight-greeter-cage-diagnostic"
    test ! -e "$stage/etc/greetd/config.toml"
    greetd_identity > /work/output/greetd-after.txt
    cmp /work/output/greetd-before.txt /work/output/greetd-after.txt
    if find "$stage" -type f \( -name 'greeter_tests' -o -name 'greeter_runtime_tests' -o -path '*/fixtures/*' \) | grep .; then exit 1; fi
    python3 scripts/check-runtime-launches.py "$stage/usr/bin/holonight-greeter" "$stage/usr" \
      --forbid-path "$PWD/build" --logs /work/output/install-launch
    python3 - "$prefix" /work/output/install-launch <<'PY_ASSERT'
from pathlib import Path
import sys
for mode in ('default', 'environment', 'command-line', 'external-config'):
    for extension in ('log', 'maps'):
        evidence = Path(sys.argv[2]) / f'{mode}.{extension}'
        assert sys.argv[1] not in evidence.read_text(), f'{mode}: installed provider build path discovered'
        assert '/prepared/' not in evidence.read_text(), f'{mode}: preparation path discovered'
PY_ASSERT
    cp -a "$stage" /work/output/stage
