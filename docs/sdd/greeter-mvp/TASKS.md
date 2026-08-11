# HoloNight Greeter MVP Tasks

| ID | State | Work |
| --- | --- | --- |
| HGR-101-SDD | Done | Specify requirements, security boundaries, architecture, and verification. |
| HGR-101-CORE | Done | Strict configuration, session/state services, account/power adapters, framed transport, and complete greetd controller flow verified. |
| HGR-101-UI | Done | Real models, dynamic prompts, responsive layout, keyboard focus, secret handling, capability gating, and confirmations verified offscreen. |
| HGR-101-INSTALL | Done | CMake installs config, wallpaper, account faces, tmpfiles, and documentation into a chosen prefix; the fallback avatar remains bundled. |
| HGR-101-TEST | Done | Domain, controller, multi-account demo discovery and switching, fragmented/coalesced transport, power-gating, deterministic scenario behavior, and the four documented offscreen demo commands pass. |
| HGR-101-VERIFY | Done | Verified 2026-08-10; local commit created after all checks below passed. |
| HGR-101-LIVE | In Progress | Isolated VT2 authentication and greetd handoff now succeed. Retry cleanup must tolerate greetd already discarding a failed PAM session. HoloNight session launch for `andrii` correctly reached UWSM but was rejected because that user's graphical session remained active on VT1; repeat with an inactive user or after logging out VT1. Cage also segfaulted during greeter teardown and needs compatibility triage. `/etc/greetd/config.toml` remains unchanged. |

Password-first follow-up: list mode now begins authentication for the saved eligible account (or the first account),
account switching completes cancellation before reconnecting, and PAM failures automatically refresh a cleared,
focused password prompt with the failure shown once. Enter remains the primary submit path and `Log in` is the only
button label. Live VT2 verification of this follow-up remains pending and must not disturb the active tux session.

Verification (2026-08-10): CMake/Ninja build and CTest, clang-format, clang-analyzer via clang-tidy, qmllint and generated
QML cache/type compilation, real demo account/avatar discovery, four offscreen demo scenarios, and temporary-prefix CMake install inspection passed. The
install includes the configured wallpaper and account faces, keeps the fallback avatar in the executable, and does not
create or modify `/etc/greetd/config.toml`. Distribution
packaging is owned by a future ecosystem-wide release initiative. Live Cage/VT verification is in progress.
