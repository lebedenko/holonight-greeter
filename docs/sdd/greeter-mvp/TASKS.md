# HoloNight Greeter MVP Tasks

| ID | State | Work |
| --- | --- | --- |
| HGR-101-SDD | Done | Specify requirements, security boundaries, architecture, and verification. |
| HGR-101-CORE | Done | Strict configuration, session/state services, account/power adapters, framed transport, and complete greetd controller flow verified. |
| HGR-101-UI | Done | Real models, dynamic prompts, responsive layout, keyboard focus, secret handling, capability gating, and confirmations verified offscreen. |
| HGR-101-INSTALL | Done | CMake installs config, wallpaper, tmpfiles, and documentation into a chosen prefix; distribution packaging is deferred to release planning. |
| HGR-101-TEST | Done | Domain, controller, multi-account demo discovery and switching, fragmented/coalesced transport, power-gating, deterministic scenario behavior, and the four documented offscreen demo commands pass. |
| HGR-101-VERIFY | Done | Verified 2026-08-10; local commit created after all checks below passed. |
| HGR-101-LIVE | Blocked | Requires explicit approval after automated verification. |

Verification (2026-08-10): CMake/Ninja build and CTest, clang-format, clang-analyzer via clang-tidy, qmllint and generated
QML cache/type compilation, real demo account/avatar discovery, four offscreen demo scenarios, and temporary-prefix CMake install inspection passed. The
install includes the configured wallpaper and does not create or modify `/etc/greetd/config.toml`. Distribution
packaging is deferred to release planning. Live Cage/VT work remains separately blocked.
