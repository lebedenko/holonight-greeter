# HoloNight Greeter MVP Specification

Status: Implemented

## Scope

HGR-101 delivers a fullscreen greetd client and a windowed, service-independent demo. Production accepts
`--config PATH` and `--state PATH`; demo accepts `--demo-scenario default|wrong-password|otp|fingerprint`.

The greeter strictly validates TOML v1 configuration, discovers eligible local users and Wayland desktop entries,
speaks framed JSON to `GREETD_SOCK`, persists only the last successful user/session, and exposes logind power actions.
Authentication is disabled on invalid configuration. Missing configuration and corrupt state are nonfatal.

## Security requirements

- Commands are parsed as desktop-entry argument vectors and are never evaluated by a shell.
- Protocol frames are bounded before allocation. Invalid data, timeout, and disconnect fail closed.
- Secret responses are never logged or persisted and mutable response buffers are wiped at every terminal boundary.
- User filters and locked-account status cannot be bypassed by explicit inclusion.
- State writes are atomic and owner-only. Manual mode neither displays nor persists a username.
- Power actions require logind capability `yes` and user confirmation; no policy rule is installed.

## Acceptance

The build, unit tests, QML lint/smoke tests, install inspection, and Arch package checks pass. The package must not own
`/etc/greetd/config.toml`. Live Cage/VT rollout is a separate, explicitly approved step.
