# Ecosystem Maintainability Standardization — holonight-greeter

| ID | Task | State | Verification |
|---|---|---|---|
| HOLONIGHT_GREETER-01 | Move QML under apps/greeter/qml (or establish that app ownership) with stable aliases and one product URI. | Planned | — |
| HOLONIGHT_GREETER-02 | Keep only system/staging installation; make umbrella coordination own cross-module removal. | Planned | — |
| HOLONIGHT_GREETER-03 | Verify `/etc/holonight/greeter.toml` remains a product sample and never replaces `/etc/greetd/config.toml`. | Planned | — |
| HOLONIGHT_GREETER-04 | Audit Holonight.Core/Controls and raw controls; add qmllint, qmltypes, install-tree, and tmpfiles tests without live login mutation. | Planned | — |

Allowed states are `Planned`, `Ready`, `In Progress`, `Done`, `Blocked`, and `Superseded`. Record exact commands
and results before marking a task `Done`.
