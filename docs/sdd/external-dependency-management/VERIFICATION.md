# EDM-019 local verification — 2026-10-14

Local acceptance passed. Publication and umbrella pinning remain separate. The original
audit baseline in SPEC.md is preserved. Source/provider/image identities, source signatures,
versions, options, acceptance input hashes and artifact hashes are in [EVIDENCE.json](EVIDENCE.json).
Full logs/artifacts remain under ignored build/ci; no host installation or live login was performed.

## Commands and results

- `python3 scripts/ci/test_launcher.py`: 12/12 pass.
- `python3 scripts/ci/run.py --prepare --provider-source-root ..`: exact committed Config then Qt,
  via `task deps PRESET=test`; final compatible reuse passes. Provider Debug builds include
  Controls (unconditional provider component) and Wayland; tests/demo/gallery OFF.
- `python3 scripts/ci/run.py`: one clean initial four-lane acceptance. Static-checks and licensing pass;
  build-test and Release revealed harness assumptions corrected below.
- `python3 scripts/ci/run.py --lane build-test --lane release`: affected clean lanes pass after corrections.
  16 real-CMake regressions pass before building; eight CTest registrations pass. Core fixtures have
  42 passes plus one expected Hyprland-parser skip in each style. Four runtime style/scale registrations
  each have 19 passes, with two historical diagnostic experiments disabled.
- Build-tree startup: all four default/environment/command-line/external-config selectors pass.
  Headless Sway/OpenGL caret pixels: Holonight/Fusion at scales 1 and 1.25 all pass.
- Release/tests-OFF: both executables, TOML configuration, wallpaper, eight faces, tmpfiles,
  greetd example, Cage guide and diagnostic executable pass payload checks. No test/fixture payload
  or staged `/etc/greetd/config.toml`. Four installed selectors pass; log/maps reject build/preparation paths.
  Image `/etc/greetd/config.toml` retains SHA-256
  `640e56c68c25fbd4da79f0e949ede6f395b1a947d50e8209a870b17456cc7891`.
- Static-checks: strict canonical format, owned-header tidy across all 15 translation units,
  strict QML lint, QML metadata/import policy and whitespace pass.
- `python3 -m unittest discover -s tooling -p 'test_*.py'` (umbrella): 32/32 pass.
- `python3 -m unittest discover -s tests -p 'test_install*.py'` (umbrella): 68/68 pass.
- `python3 tooling/generate-install-profile.py --check`, `python3 tooling/sync.py --check --module holonight-greeter`,
  canonical declaration byte equality, `external.py validate` and repository `git diff --check`: pass.
- `task --dry caret-graphics`: correct explicit test preset/runtime executable and prepared prefix.

Accepted logs: static/licensing in `build/ci/20261014T201749Z-vorsms1d/`;
build-test/Release in `build/ci/20261014T202413Z-6wpnkpt7/`. The initial failed lanes are retained,
not misrepresented as passing. Corrected lanes use the unchanged qualified full Arch digest.

## Review and corrections

The isolated Arch package-query account must include `alpm`. Sway's installed file capability
cannot execute under Docker's default bounding set: a private byte-for-byte executable copy
omits capability xattrs and requires no extra privileges. The qualified image includes greetd's
configuration, so Release checks preservation by hash rather than requiring initial absence.
The developer caret helper now explicitly builds the test preset. No production C++ or QML,
fixture assertions, authentication/transport/session contracts or install discovery behavior changed.

Expected diagnostics: Qt private-ABI notices, optional missing Vulkan headers, and import-scanner
notices for the generated Holonight namespace parent. Strict lint/types plus real startup origins
verify discovery despite that scanner notice. Tidy filters foreign headers and retains existing
NOLINT annotations; owned code has no actionable diagnostics. Headless Sway logs missing optional
Xwayland/background helpers; these are unused by the four passing native Wayland caret cases.
Hyprland-parser skip remains explicit; native compositor qualification is EDM-022.

Documentation/Task-only closure does not invalidate application/static acceptance. Compatible
provider preparation is refreshed for the final Task input; licensing and whitespace are checked
again after documentation. No broad compatibility cleanup was needed.

Final preparation reuse: `build/ci/20261014T204846Z-j7vhtsm1/` (pass).
Final pinned REUSE/documentation check: `build/ci/20261014T205024Z-ntky1u1g/` (pass).
Recorded artifact hashes and local SDD links were checked against the retained files.
