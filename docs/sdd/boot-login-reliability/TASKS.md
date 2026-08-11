# Boot and Login Reliability Tasks

| ID | State | Work |
| --- | --- | --- |
| HGR-102-DIAG | Done | Add privacy-bounded lifecycle/protocol logging and the opt-in Cage diagnostic launcher. |
| HGR-103-ADAPTERS | Done | Add the typed Hyprland/Cage launcher, isolated Hyprland config, explicit Cage override, layouts, deterministic output selection, and one-credential-surface multi-output composition. |
| HGR-103-REPO-VERIFY | Done | Build, 32-test CTest suite, formatting, qmllint, diff check, Hyprland Lua parser validation, and temporary-prefix installation passed on 2026-08-11. Unix-socket tests required running outside the managed sandbox. |
| HGR-103-NESTED | Planned | Run nested Hyprland multi-output, layout IPC, termination, and handoff tests. |
| HGR-103-PHYSICAL | Done | User verified the isolated VT2 launcher on 2026-08-11 with `eDP-1` and `DP-5`: both outputs showed the configured wallpaper and exactly one showed the login panel. The final run used `start-hyprland`, the private `hyprland.lua`, and output-bound layer-shell surfaces without the earlier parser, launcher, or solid-background failures. |
| HGR-102-REPO-VERIFY | Done | CMake build, CTest, `task lint`, launcher syntax validation, and a temporary `/usr`-prefix install passed on 2026-08-11. The install contains the executable, `/etc` config, tmpfiles rule, deployment docs, assets, and libexec diagnostic launcher. |
| HGR-102-LIVE-HARNESS | Done | `task live:test` builds under `/tmp`, runs the installed `/usr/bin/holonight-greeter-session --config /etc/holonight/greeter.toml`, reserves VT2 without changing primary greetd, restores getty, enables cores, and records foreground output. Cage rescue remains an explicit `--backend cage` invocation. |
| HGR-102-CAGE | Planned | Reproduce packaged/current Cage on an isolated VT, symbolize the core, select the deterministic patch path, and file upstream. |
| HGR-102-LIVE | Planned | User performs ten isolated-VT login cycles covering success, retry, account switch, cancellation, and restart. |
| HGR-102-HOST | Planned | With explicit approval, clean the kernel command line, create `plugdev`, reboot, and verify boot/network state. |
| HGR-102-FINAL | Planned | Record the final boot ID, date, Cage package revision, journal/core results, and acceptance outcome below. |

## Final verification record

Pending. Do not mark the recurrence resolved from nested or repository-only checks.
