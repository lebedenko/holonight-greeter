# HoloNight Greeter MVP Tasks

| ID | State | Work |
| --- | --- | --- |
| HGR-101-SDD | Done | Specify requirements, security boundaries, architecture, and verification. |
| HGR-101-CORE | Done | Strict configuration, session/state services, account/power adapters, framed transport, and complete greetd controller flow verified. |
| HGR-101-UI | Done | Real models, dynamic prompts, responsive layout, keyboard focus, secret handling, capability gating, and confirmations verified offscreen. |
| HGR-101-INSTALL | Done | CMake installs config, wallpaper, account faces, tmpfiles, and documentation into a chosen prefix; the fallback avatar remains bundled. |
| HGR-101-TEST | Done | Domain, controller, multi-account demo discovery and switching, fragmented/coalesced transport, power-gating, deterministic scenario behavior, and the four documented offscreen demo commands pass. |
| HGR-101-VERIFY | Done | Verified 2026-08-10; local commit created after all checks below passed. |
| HGR-101-LIVE | Done | User-confirmed isolated VT2 verification completed on 2026-08-11 after the password-first recovery fixes: saved-user startup accepted immediate password entry and Enter, account switching focused a cleared password field, a wrong password showed one error and refreshed the prompt, and a following correct password started the selected HoloNight session. The previously observed cancellation cleanup and Cage teardown issues no longer reproduced. `/etc/greetd/config.toml` remained unchanged. |

Password-first follow-up: list mode now begins authentication for the saved eligible account (or the first account),
account switching completes cancellation before reconnecting, and PAM failures automatically refresh a cleared,
focused password prompt with the failure shown once. Enter remains the primary submit path and `Log in` is the only
button label. Live VT2 verification of this follow-up remains pending and must not disturb the active tux session.

Verification (2026-08-10): CMake/Ninja build and CTest, clang-format, clang-analyzer via clang-tidy, qmllint and generated
QML cache/type compilation, real demo account/avatar discovery, four offscreen demo scenarios, and temporary-prefix CMake install inspection passed. The
install includes the configured wallpaper and account faces, keeps the fallback avatar in the executable, and does not
create or modify `/etc/greetd/config.toml`. Distribution
packaging is owned by a future ecosystem-wide release initiative. Final live Cage/VT verification passed on
2026-08-11 with the user confirming that all known greeter issues were fixed.
