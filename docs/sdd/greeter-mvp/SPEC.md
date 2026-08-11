# HoloNight Greeter MVP Specification

Status: Implemented

## Scope

HGR-101 delivers a fullscreen greetd client and a windowed demo with simulated authentication and system actions. Production accepts
`--config PATH` and `--state PATH`; demo accepts `--demo-scenario default|wrong-password|otp|fingerprint`.

The greeter strictly validates TOML v1 configuration, discovers eligible local users and Wayland desktop entries,
speaks framed JSON to `GREETD_SOCK`, persists only the last successful user/session, and exposes logind power actions.
Authentication is disabled on invalid configuration. Missing configuration and corrupt state are nonfatal.
Demo reuses the configured read-only account and session discovery boundaries, falling back to the process account
and a synthetic session when discovery is empty. It never contacts greetd or logind and never persists state.

## Security requirements

- Commands are parsed as desktop-entry argument vectors and are never evaluated by a shell.
- Protocol frames are bounded before allocation. Invalid data, timeout, and disconnect fail closed.
- Secret responses are never logged or persisted and mutable response buffers are wiped at every terminal boundary.
- User filters and locked-account status cannot be bypassed by explicit inclusion.
- State writes are atomic and owner-only. Manual mode neither displays nor persists a username.
- Power actions require logind capability `yes` and user confirmation; no policy rule is installed.

## Acceptance

The build, unit tests, QML lint/smoke tests, and temporary-prefix CMake install inspection pass. CMake installation
must not create or modify `/etc/greetd/config.toml`. Per-repository distribution packaging is outside this MVP and is
owned by a future ecosystem-wide release initiative. Live Cage/VT rollout is a separate, explicitly approved step.
