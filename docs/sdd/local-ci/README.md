# Local CI rehearsal

Baseline f0ecd224ad991c0ec13ff472fd015174eac150ee. Umbrella CI-013.

Preserve combined build/test/static/package job and separate licensing, including
push/PR triggers and results. Immutable build/REUSE 6.2.0 environments use pinned
ripgrep, DejaVu fonts and layer-shell-qt 6.7.4-1 (built against matching Qt 6.11.1).
Provider Config fe69a59e6b73167fd5349223a4d265d75386c139 / Qt
0e0f92efe1517bc07c577299f07b51117f25f05a unchanged. Fresh Release providers,
Debug BUILD_TESTING=ON, full CTest styles/scales/policy fixtures, format/QML/static
metadata analysis, demo-only isolated build/installed startup and all staged
payload/config safety assertions remain required. Reject external provider paths
from installed evidence. Never run production greetd or modify host configuration.

Read-only HEAD archive plus current-input snapshot enables meaningful pinned-Git
whitespace checks without writable Git metadata: git diff --no-index --check
compares edits/deletions/new files; exit 1 means clean differences, errors fail.
Preserve modes/symlinks; include/report non-ignored new inputs. Disposable working
source/provider/build trees, host-owned logs/evidence and identity/state/results
in ignored build/ci; unavailable required checks fail. No publication/pin updates.

Resolve host clang-tidy 23 diagnostics while preserving architecture/public Qt APIs
and full check families. Own-path header filters exclude foreign providers. Ignore
only single-line trailing comma policy due to the documented clang 23 delimiter
misidentification; retain multiline validation.

Accepted locally on 2026-10-04. `python3 build/ci/acceptance.py` ran full
`task ci` with exit 0: both required lanes pass. Evidence:
`build/ci/20261003T221250Z-309e66tf/` (complete logs, results, eight startup
mode log/map pairs, Testing output). All eight CTest registrations pass;
131 source/development files are unchanged and 29 evidence files are host-owned.
Five launcher regressions cover snapshots, whitespace, missing runtimes and
failure propagation. Real Podman is unavailable; its argument mapping is tested.

Fresh native Debug build with exact Release providers also passes full owned
clang-tidy 23.1.1, format-check, REUSE and eight offscreen CTest registrations.
Native socket tests require sandbox escalation; initial sandbox failures are
retained separately. Logs are `build/ci/native-*-final.log` and exact provider
provenance is `build/ci/native-providers/provenance.json`. Full string-literal
comparison against baseline passes: QML/config/protocol names remain unchanged.

Owned diagnostic corrections format existing source/tests, use explicit
initializers/conditions and internal field names, and separate configuration
sections, account collection, Exec expansion, record initialization, demo/auth
prompt handling and caret pixel assertions. Preserve validation order, wire
framing, state transitions and every existing pixel assertion; disabled visual
experiments remain disabled. Header filters cover only owned source/tests;
format validation now includes test headers. Expected Qt-private ABI notices
remain visible; the layer-shell supplement matches the pinned Qt build.
No production greeter, host configuration changes, publication or pin updates.
