# UQC-106 implementation record

Status: Complete — local acceptance and remote greeter CI/licensing passed.

## Baselines and publication order

2026-09-09: Canonical starting revisions matched [SPEC](SPEC.md). Greeter had no
local AGENTS.md; umbrella instructions apply. Greeter design `7bd4bdf` and linked
umbrella In Progress checkpoint `f4fa526` were published before implementation.

Initial acceptance exposed a provider bug: empty iconRole looked up the entire
QVariantMap row, emitting `Unable to assign QVariantMap to QUrl`. The user approved
UQC-109. Its provider regression failed before the fix and passes afterward.
Provider implementation `7ee28b1` and completed handoff
`00e6e208b6c9b30d89b66ef3aeb4ef8175050764` are published and canonically confirmed.
Umbrella checkpoint `ef7d824` pinned that handoff before greeter adopted it.
Provider CI `34288860789` and licensing `34288860847` pass at the final prerequisite.
Configuration remains `fe69a59e6b73167fd5349223a4d265d75386c139`, unchanged.

## Changed files and behavior

- `qml/Main.qml`, `LoginPanel.qml`: qualified runtime standard controls, enums and
  attached properties; explicit Core import. Core/composite visuals are preserved.
- `qml/FooterSelector.qml`: public HnIconComboBox, `iconRole: ""`, inherited scaled
  popup, `delegateHeight: height`, existing roles/handlers and semantic painting.
- `qtquickcontrols2.conf`, `src/main.cpp`, `CMakeLists.txt`: embedded overridable
  Holonight default and discovery for the exact build executable versus executable-
  relative installed QML. QML/native paths come from configured provider packages.
- `Taskfile.yml`, `Dockerfile.ci`, `.github/workflows/ci.yml`: matching pinned private
  dependency builds, fonts, generated artifacts before static checks, all four QML
  files, runtime/policy/launch checks and retained installation-content assertions.
- `tests/runtime/`: separate GUI executable sharing production QML/assets, real
  controller, fake services and temporary hyprctl. Core test sources are unchanged.
- `scripts/check-runtime-qml-imports.sh`, `check-runtime-launches.py`,
  `run-isolated-test.py`, `check-static.py`, `.clang-tidy`: policy fixtures,
  isolated bounded/reaped subprocesses and static analysis. README documents the
  private dependency workflow, style overrides and development native discovery.

No configuration repository/schema/shared setting/TOML-format changes, controller
or service API changes, output-manager changes, session-helper changes, system
installation, live authentication/session/power actions or desktop pointer/focus
automation. The package-manager's two untracked mockups remain untouched.

## Verification (2026-09-09, Qt 6.11.2)

From holonight-greeter:

```sh
task build BUILD_DIR=build-uqc
cmake --build build-uqc -j4
ctest --test-dir build-uqc -R '^runtime_Holonight_1$' --output-on-failure
bwrap --bind / / --tmpfs /usr/lib/qt6/qml/Holonight --ro-bind /dev/null /usr/lib/libholonight_config.so --dev /dev --proc /proc ctest --test-dir build-uqc --output-on-failure
cmake --build build-uqc --target format-check qml-lint
python3 scripts/check-static.py build-uqc
DESTDIR=/tmp/holonight-greeter-uqc-stage cmake --install build-uqc
cmake --install build-uqc/dependencies/config --prefix /tmp/holonight-greeter-uqc-stage/usr
cmake --install build-uqc/dependencies/qt --prefix /tmp/holonight-greeter-uqc-stage/usr
bwrap --bind / / --tmpfs /usr/lib/qt6/qml/Holonight --ro-bind /dev/null /usr/lib/libholonight_config.so --dev /dev --proc /proc python3 scripts/check-runtime-launches.py build-uqc/holonight-greeter build-uqc/dependencies/prefix --logs build-uqc/launch-build-fixed
bwrap --bind / / --tmpfs /usr/lib/qt6/qml/Holonight --ro-bind /dev/null /usr/lib/libholonight_config.so --dev /dev --proc /proc python3 scripts/check-runtime-launches.py /tmp/holonight-greeter-uqc-stage/usr/bin/holonight-greeter /tmp/holonight-greeter-uqc-stage/usr --forbid-path build-uqc --logs build-uqc/launch-installed-fixed
reuse lint
git diff --check
```

The full host-masked suite passes all 8 CTest entries (16.10 seconds): 43 existing
core/CLI tests under each style, 9 GUI acceptance tests in each of four Holonight/Fusion × DPR 1/1.25
processes, import policy and 15 independently evaluated policy fixtures. New
asynchronous tests use GTest assertions around bounded QTest waits. The core
socket tests run outside the sandbox; native dependencies stay privately staged.

GUI acceptance checks actual control implementation URLs and plugin maps;
Core/composite painting; users/sessions, manual entry, password masking, OTP,
fingerprint, wrong-password recovery, cancellation, successful fake session flow,
configuration errors, disabled states, power confirmation and selector IPC
arguments/success/failure. A separate wallpaper engine loads a temporary image.
Layout/popup assertions cover 1672×941, 899/900 breakpoints, 2200×1200, panel scales
0.78/1/1.25, above/below edge placement, actual delegate height, popup scaling,
overflow, selected-row visibility and first/last-row reachability. Both styles and
DPRs pass without unexpected QML diagnostics after the provider correction.

All eight actual build/install launch selectors pass: embedded Holonight,
environment Fusion, command-line Fusion over environment Holonight, and external
Fusion configuration. Each process stays alive for three seconds and supplies
qml-loaded, selected control URL and plugin-map evidence before termination/reaping.
Installed evidence excludes build paths. Both matrices pass with host provider QML
and native configuration libraries hidden. Installation-content assertions pass:
both executables, wallpaper, eight faces, tmpfiles, configuration, documentation,
diagnostic wrapper, and absence of an installed greetd configuration.

Formatting, zero-warning four-file QML lint and generated type metadata pass.
Clang-analyzer checks cover all 15 owned C++ translation units, using a temporary
analysis database without GCC-only flags. Python/YAML/workflow shell syntax,
Taskfile parsing, local SDD links, REUSE licensing and whitespace pass.

## Published acceptance and remaining ecosystem gates

Greeter implementation `dbc86f34b6ad0d6eb75351eedacc08f756b82d97` is published and
canonically confirmed. CI `34292227990` passes its complete build/test/static/
launch/install job (6 minutes 40 seconds); licensing `34292227948` passes.
This documentation handoff closes UQC-106 local acceptance; its authoritative
revision is the umbrella gitlink after publication confirmation. The final
umbrella checkpoint records remote checks for this documentation revision.

Shell canonical main was rechecked as `723763e09ff815d344a6cb01529dd8345a43b316`;
its checkout is clean and AGENTS.md was read. The next Ready UQC-102 assignment
uses the corrected, published provider prerequisite `00e6e20`.
UQC-201 remains Planned and the initiative remains Accepted; human-operated
pre-session/compositor and ecosystem integration are not claimed by local tests.
