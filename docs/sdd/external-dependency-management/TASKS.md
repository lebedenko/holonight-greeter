# EDM-019 tasks

Implementation repository: holonight-greeter only. Audit baseline remains
`d99e0a17f95af977a42785957e9311597e38bab0`; implementation starts at
`31e8f8ece6d4bb4e90d11433a253150726535916`. Exact coordinator checkpoint:
`ab22e74bd8d5660b599d439c4887bed088f90637`.

| Task | State | Evidence |
| --- | --- | --- |
| Copy canonical declarations and synchronize bundle 2.0.0 | Done | Byte equality, bundle drift and generated profile checks |
| Require installed GTest CONFIG and exact private Qt | Done | 16 real-CMake regressions pass |
| Make task deps the sole ordered provider preparation command | Done | Exact Config/Qt preparation, compatible artifact reuse |
| Replace project images/bootstrap with qualified offline lanes | Done | Read-only installed providers, caller UID/GID, no network/pulls in acceptance |
| Preserve strict format/tidy, QML and licensing | Done | 15 owned units, QML metadata/lint/import policy, REUSE |
| Preserve eight CTest registrations and authentication/session/transport fixtures | Done | Eight registrations pass, 42 core passes per style plus expected parser skip, 19 runtime passes per style/scale |
| Run headless caret checks and installed Release startup/payload checks | Done | Four caret cases and clean Release/installed selectors pass |
| Review evidence, commit and deliver local handoff | Done | Local verification and reviewed repository commit; publication and umbrella pinning are separate |

Daemon, Hyprlock and final ecosystem integration are outside this package. Native compositor
verification remains EDM-022. Host installation, live greetd/VT checks, desktop automation
and Debian qualification are excluded.
