# HoloNight Greeter MVP Tasks

| ID | State | Work |
| --- | --- | --- |
| HGR-101-SDD | Done | Specify requirements, security boundaries, architecture, and verification. |
| HGR-101-CORE | Done | Strict configuration, session/state services, account/power adapters, framed transport, and complete greetd controller flow verified. |
| HGR-101-UI | Done | Real models, dynamic prompts, responsive layout, keyboard focus, secret handling, capability gating, and confirmations verified offscreen. |
| HGR-101-PKG | Done | Package installs config, wallpaper, tmpfiles, and documentation; package content and ownership assertions pass. |
| HGR-101-TEST | Done | Domain, controller, fragmented/coalesced transport, power-gating, and four-scenario offscreen tests pass. |
| HGR-101-VERIFY | Done | Verified 2026-08-10; local commit created after all checks below passed. |
| HGR-101-LIVE | Blocked | Requires explicit approval after automated verification. |

Verification (2026-08-10): CMake/Ninja build and CTest, clang-format, clang-analyzer via clang-tidy, qmllint and generated
QML cache/type compilation, four offscreen demo scenarios, temporary DESTDIR inspection, and `makepkg --nodeps` passed.
The package has one backup (`etc/holonight/greeter.toml`), includes the configured wallpaper, and does not own
`/etc/greetd/config.toml`. `--nodeps` was required because the locally installed HolonightQt baseline is not registered
as the Arch package `holonight-qt`. Live Cage/VT work remains separately blocked.
