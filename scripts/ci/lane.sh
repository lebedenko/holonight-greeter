#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
lane=$1
mkdir /work/source
cp -a /input/. /work/source/
cd /work/source
export HOME=/work/build/home LC_ALL=C.UTF-8 TZ=UTC
export JOBS=${JOBS:-2} CMAKE_BUILD_PARALLEL_LEVEL=${JOBS:-2}
export QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software QT_FORCE_STDERR_LOGGING=1
export QT_QPA_PLATFORMTHEME= QT_STYLE_OVERRIDE= XDG_RUNTIME_DIR=/work/runtime
mkdir -p "$HOME"
mkdir -m 700 "$XDG_RUNTIME_DIR"
if [ "$lane" = licensing ]; then
  reuse --version
  reuse lint
  exit
fi
mkdir -p build/ci-provenance
pacman -Q > build/ci-provenance/packages.txt
c++ --version > build/ci-provenance/compiler.txt
cmake --version > build/ci-provenance/cmake.txt
{ python3 --version; task --version; ninja --version; clang-format --version; clang-tidy --version; qmake6 --version; wayland-scanner --version; } > build/ci-provenance/versions.txt 2>&1
capture() {
  status=$?
  cp -a build/ci-provenance /output/
  if [ -d build/test/Testing ]; then cp -a build/test/Testing /output/; fi
  if [ -f build/test/compile_commands.json ]; then cp build/test/compile_commands.json /output/; fi
  if [ -d .cache/tooling ]; then cp -a .cache/tooling /output/tooling; fi
  for preset in test release; do
    if [ -f "build/$preset/CMakeCache.txt" ]; then
      mkdir -p "/output/ci-provenance/$preset"
      cp "build/$preset/CMakeCache.txt" "/output/ci-provenance/$preset/"
      for metadata in build/$preset/Holonight/Greeter/*.qmltypes build/$preset/runtime-qml/*.qmltypes; do
        if [ -f "$metadata" ]; then cp "$metadata" "/output/ci-provenance/$preset/"; fi
      done
    fi
  done
  exit "$status"
}
trap capture 0
if [ "$lane" = prepare ]; then
  export HOLONIGHT_CONFIG_SOURCE=/prepared/sources/holonight-config
  export HOLONIGHT_QT_SOURCE=/prepared/sources/holonight-qt
  if [ -d /prepared/deps ]; then cp -a /prepared/deps build/deps; fi
  task deps PRESET=test
  mkdir -p /prepared/deps
  cp -a build/deps/. /prepared/deps/
  cp -a build/ci-provenance /prepared/environment
  exit
fi
cp -a /prepared/deps build/deps
cp /prepared/provider-revisions.json build/ci-provenance/
cp build/deps/provider-revisions.tsv build/ci-provenance/
prefix=$PWD/build/deps/prefix
export LD_LIBRARY_PATH="$prefix/lib"
build=build/test
stage=/work/install
case "$lane" in
  build-test)
    python3 scripts/ci/test_launcher.py
    python3 scripts/ci/test_dependencies.py
    task build PRESET=test
    ctest --test-dir "$build" --show-only=json-v1 > /output/tests.json
    ctest --test-dir "$build" --output-on-failure --no-tests=error
    cat "$build/Testing/Temporary/LastTest.log"
    python3 - /output/tests.json <<'PY_TESTS'
import json, sys
expected = {'greeter_tests', 'greeter_tests_Fusion', 'runtime_Holonight_1',
            'runtime_Holonight_1.25', 'runtime_Fusion_1', 'runtime_Fusion_1.25',
            'runtime_import_policy', 'runtime_import_policy_fixtures'}
assert {t['name'] for t in json.load(open(sys.argv[1]))['tests']} == expected
PY_TESTS
    python3 scripts/check-runtime-launches.py "$build/holonight-greeter" "$prefix" --logs /output/build-launch
    python3 scripts/check-caret-graphics.py "$build/greeter_runtime_tests" "$prefix" --logs /output/caret-graphics
    ;;
  static-checks)
    task build PRESET=test
    task format-check
    python3 scripts/check-static.py "$build"
    cmake --build "$build" --target qml-lint
    task qml-import-check
    task qmltypes-check
    git diff --check HEAD
    ;;
  release)
    task build PRESET=release
    greetd_identity() {
      if [ -e /etc/greetd/config.toml ]; then sha256sum /etc/greetd/config.toml; else printf 'absent\n'; fi
    }
    greetd_identity > /output/greetd-before.txt
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
    greetd_identity > /output/greetd-after.txt
    cmp /output/greetd-before.txt /output/greetd-after.txt
    if find "$stage" -type f \( -name 'greeter_tests' -o -name 'greeter_runtime_tests' -o -path '*/fixtures/*' \) | grep .; then exit 1; fi
    python3 scripts/check-runtime-launches.py "$stage/usr/bin/holonight-greeter" "$stage/usr" \
      --forbid-path "$PWD/build" --logs /output/install-launch
    python3 - "$prefix" /output/install-launch <<'PY_ASSERT'
from pathlib import Path
import sys
for mode in ('default', 'environment', 'command-line', 'external-config'):
    for extension in ('log', 'maps'):
        evidence = Path(sys.argv[2]) / f'{mode}.{extension}'
        assert sys.argv[1] not in evidence.read_text(), f'{mode}: installed provider build path discovered'
        assert '/prepared/' not in evidence.read_text(), f'{mode}: preparation path discovered'
PY_ASSERT
    cp -a "$stage" /output/stage
    ;;
  *) echo "Unknown lane: $lane" >&2; exit 2 ;;
esac
