# UQC-106 design

Standard controls, enums and attached properties use `QtQuick.Controls as Controls`.
Keep Holonight.Core and Holonight.Controls. FooterSelector uses HnIconComboBox,
`delegateHeight: height` and `iconRole: ""`, retaining content, indicator,
background, roles and selection handlers. Reuse provider scale/placement behavior.

Embed `:/qtquickcontrols2.conf` with `[Controls]` / `Style=Holonight` in each
GUI executable. Do not call imperative style selection. Derive dependency QML
and native discovery from configured packages. Only the exact build executable
may add its build dependency path; installed execution resolves QML relative to
its executable. The QtQuick-only wallpaper engine needs no provider injection.
The non-graphical session helper is unchanged.

Build pinned dependencies inside greeter-owned directories and stage privately.
Disable provider tests/examples while retaining Wayland. Align Taskfile and CI,
include fonts, build generated artifacts before static analysis and retain all
installation-content assertions. Keep runtime policy fixtures independently failing.

Share production QML/assets with a separate GUI acceptance executable. Use the
real controller and existing fake service pattern. Drive properties/signals in
an offscreen process, never desktop input automation. Temporary hyprctl captures
arguments and supplies deterministic success/failure without active compositor IPC.
Actual executable probes use demo mode with disposable paths and unavailable
services, assert implementation/plugin traces and reject early exits or absent
evidence. Every subprocess is bounded and reaped. No production diagnostic API.

See [requirements](SPEC.md) and [verification ledger](TASKS.md).
