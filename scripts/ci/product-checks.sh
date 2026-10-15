#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
build=build/dev
prefix="$PWD/build/deps/prefix"
stage="$PWD/build/installed"
mkdir -p /work/output
    python3 scripts/check-runtime-launches.py "$build/holonight-greeter" "$prefix" --logs /work/output/build-launch
    python3 scripts/check-caret-graphics.py "$build/greeter_runtime_tests" "$prefix" --logs /work/output/caret-graphics
