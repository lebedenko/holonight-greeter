# UQC-106: Runtime Quick Controls

Status: Local implementation and acceptance complete; remote publication checks pending.

## Scope and baselines

Greeter baseline: `9130c9ccbf05986ab1843ae322831e7fe9efdac9`.
Provider: `00e6e208b6c9b30d89b66ef3aeb4ef8175050764` (authorized UQC-109 repair
of initial prerequisite `478ef7c40a22c7c3f7ea6f45d9205411b5504834`).
Configuration: `fe69a59e6b73167fd5349223a4d265d75386c139` (unchanged).
Umbrella assignment: `1219422c9372753996356263785327becefa33fa`.

Use namespaced runtime Qt Quick Controls and an embedded, overridable Holonight
style default. Preserve authentication, sessions, output management, deployment,
Core/composite visuals and configuration interfaces. FooterSelector adopts the
public HnIconComboBox with text-only popup rows and its existing scaled geometry.
No configuration repository, schema, shared setting or TOML-format changes.

## Acceptance

- Separate GUI acceptance loads production QML/assets with real controller and fake
  services; retain the existing core tests. Verify actual control implementation
  URLs and native plugin origins under fresh Holonight/Fusion processes.
- Cover users, sessions, manual entry, masking, wrong-password recovery, OTP,
  fingerprint, cancellation, configuration errors, disabled states and power
  confirmation. Substitute hyprctl privately and assert selector IPC/failure.
- Verify 1672×941, widths 899/900, representative scales, DPR 1/1.25, delegate
  heights, popup edge placement, overflow, selected-row visibility and row reachability.
- Verify secondary wallpaper QML in its own engine with a temporary image.
- Launch actual build and private installed executables with embedded default,
  environment Fusion, command-line Fusion over environment Holonight, and external
  Fusion configuration. Isolate HOME/XDG/state/configuration and desktop services;
  bound observation, require loading evidence, terminate and reap each process.
- Repeat discovery with host HoloNight QML/native libraries hidden. Installed
  execution must not discover build dependencies.
- Pass focused and full tests, both styles, formatting, clang-tidy, four-file QML
  lint/types, independent policy fixtures, installation contents, syntax, links,
  whitespace, canonical publication, remote CI and licensing.

No system installation, live authentication/session/power actions or desktop
pointer/focus automation. Real compositor/pre-session and final ecosystem checks
remain human-operated UQC-201 integration gates.

See [design](DESIGN.md), [tasks](TASKS.md), and [record](IMPLEMENTATION.md).
