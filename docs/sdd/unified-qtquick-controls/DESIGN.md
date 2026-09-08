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

## Resolved provider incompatibility (UQC-109)

Opening FooterSelector with the real controller’s QVariantList model and
`iconRole: ""` emits `Unable to assign QVariantMap to QUrl` from
`HnIconComboBox.qml:199`. Qt's delegate model returns the row for `rowModel[""]`.
The existing `roleValue()` helper already returns an empty string for empty roles,
but the delegate's first branch bypasses that guard.

User-authorized provider correction: guard the delegate lookup with
`root.iconRole.length > 0 && rowModel && rowModel[root.iconRole] !== undefined`,
then keep its existing roleValue fallback. Provider regression coverage verifies
empty icon roles with QVariantList rows under Holonight/Fusion. Published provider
handoff `00e6e208b6c9b30d89b66ef3aeb4ef8175050764` is the corrected prerequisite.
No configuration API or format change is needed. A consumer delegate replacement
would duplicate the provider’s painting and bypass the planned public composite.
