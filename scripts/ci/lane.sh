#!/bin/sh
set -eu
lane=$1
mkdir /work/source
cp -a /input/. /work/source/
cd /work/source
export HOME=/work/build/home LC_ALL=C.UTF-8 TZ=UTC
mkdir -p "$HOME"
if [ "$lane" = licensing ]; then
  reuse --version
  reuse lint
  exit
fi
python3 --version
git --version
cmake --version
ninja --version
c++ --version
clang-format --version
clang-tidy --version
pkg-config --modversion Qt6Core Qt6Quick
rg --version
python3 scripts/ci/test_launcher.py
python3 scripts/ci/check-whitespace.py /baseline /input
export QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software QT_FORCE_STDERR_LOGGING=1
export XDG_RUNTIME_DIR=/work/runtime
mkdir -m 700 "$XDG_RUNTIME_DIR"
mkdir -p /work/providers
fetch_provider() {
  name=$1
  revision=$2
  git init -q "/work/providers/$name"
  git -C "/work/providers/$name" fetch --depth 1 "https://github.com/lebedenko/$name.git" "$revision"
  git -C "/work/providers/$name" checkout --detach FETCH_HEAD
  [ "$(git -C "/work/providers/$name" rev-parse HEAD)" = "$revision" ]
}
fetch_provider holonight-config fe69a59e6b73167fd5349223a4d265d75386c139
fetch_provider holonight-qt 0e0f92efe1517bc07c577299f07b51117f25f05a
prefix=/work/providers/prefix
cmake -S /work/providers/holonight-config -B /work/providers/config-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr
cmake --build /work/providers/config-build --parallel 2
cmake --install /work/providers/config-build --prefix "$prefix"
cmake -S /work/providers/holonight-qt -B /work/providers/qt-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$prefix" \
  -DBUILD_TESTS=OFF -DBUILD_DEMO=OFF -DBUILD_CONTROLS_GALLERY=OFF -DBUILD_WAYLAND=ON
cmake --build /work/providers/qt-build --parallel 2
cmake --install /work/providers/qt-build --prefix "$prefix"
build=build/verification
trap 'status=$?; if [ -d "$build/Testing" ]; then cp -a "$build/Testing" /output/; fi; exit "$status"' 0
cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_TESTING=ON -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_PREFIX_PATH="$prefix" -DHolonightQt_DIR="$prefix/lib/cmake/HolonightQt"
cmake --build "$build" --parallel 2
export LD_LIBRARY_PATH="$prefix/lib"
ctest --test-dir "$build" --output-on-failure --no-tests=error
cmake --build "$build" --target format-check
cmake --build "$build" --target qml-lint
python3 scripts/check-static.py "$build"
python3 scripts/check-runtime-launches.py "$build/holonight-greeter" "$prefix" --logs /output/build-launch
test ! -e /etc/greetd/config.toml
stage=/work/install
DESTDIR="$stage" cmake --install "$build"
cmake --install /work/providers/config-build --prefix "$stage/usr"
cmake --install /work/providers/qt-build --prefix "$stage/usr"
python3 scripts/check-runtime-launches.py "$stage/usr/bin/holonight-greeter" "$stage/usr" \
  --forbid-path "$PWD/build" --logs /output/install-launch
python3 - "$prefix" /output/install-launch <<'PY_ASSERT'
from pathlib import Path
import sys
for mode in ('default', 'environment', 'command-line', 'external-config'):
    for extension in ('log', 'maps'):
        evidence = Path(sys.argv[2]) / f'{mode}.{extension}'
        assert sys.argv[1] not in evidence.read_text(), f'{mode}: installed provider build path discovered'
PY_ASSERT
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
test ! -e /etc/greetd/config.toml
