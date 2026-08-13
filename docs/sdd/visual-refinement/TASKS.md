# HGR-104 Visual Refinement Tasks

| ID | State | Work |
| --- | --- | --- |
| HGR-104-SDD | Done | Add the visual-refinement specification, design, task split, assumptions, and verification boundaries. |
| HGR-104-BACKEND | Done | Add arbitrary layout selection, authentication normalization, and logind session-derived power confirmation contracts. |
| HGR-104-UI | Done | Unify footer selectors, remove accessibility UI, style system-action interaction states, and add inline power confirmation. |
| HGR-104-COMBOBOX | Done | Require `HolonightQt 0.1.1` and remove footer popup geometry overrides. Isolated-prefix build, 34 non-socket tests, QML compile/lint, 8 demo smoke tests, and diff check passed 2026-08-13; two `QLocalServer::listen` tests were environment-blocked on repeat. User confirmed the installed selector popup on the physical display. |
| HGR-104-AUTO | Done | Build, 35-test CTest suite (including offscreen demo scenarios), formatting, qmllint/QML compilation, and diff checks passed on 2026-08-12. |
| HGR-104-VISUAL | Planned | Inspect selector geometry/fonts/chevrons and all hover, pressed, focus, Yes/No, and Escape states in demo mode. |
| HGR-104-VT | Planned | On an isolated VT, verify direct power with no normal user session and confirmation with another user session; never disturb an unrelated session. |

## Assumptions

- A logged-in normal user means any logind session whose class is exactly `user`, regardless of locality, activity,
  or foreground state.
- Demo mode represents the guarded confirmation case.
- Compact-mode visibility and the existing panel geometry remain unchanged.
