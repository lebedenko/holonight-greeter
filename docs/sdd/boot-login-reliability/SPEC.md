# Boot and Login Reliability Specification

## Problem

After MVP verification, Cage teardown crashes and intermittent greeter exits recurred. The next failure must leave
enough non-sensitive evidence to distinguish a greeter, greetd transport, session launcher, compositor, or DRM
lifecycle failure.

## Requirements

- Log application startup and normal exit, QML load outcome, controller state transitions, greetd request/reply
  types, transport failure categories, and successful session start at journal-visible levels.
- Never log usernames, passwords, prompt text or responses, authentication JSON payloads, session commands, or state
  file contents and paths.
- Provide an opt-in Cage diagnostic launcher that emits plain-text version/start/exit metadata, enables wlroots
  backend/output discovery, and leaves Cage stderr attached without filtering or rewriting it.
- Install a typed, fixed-command launcher with Hyprland as release default and Cage as explicit rescue backend. Do
  not execute configured commands or fall back automatically.
- Generate isolated Hyprland configuration under `/run`, cover every output with one credential surface total, and
  select that surface by connected configured connector, compositor/Qt primary, then compositor discovery order.
- Create output windows hidden, assign screens before mapping, and fail startup visibly if any wallpaper component
  cannot load.
- Support ordered XKB layouts with a valid default. Hyprland cycles through bounded fixed IPC; Cage remains fixed.
- Preserve the legacy keyboard label when no layout list is supplied.
- Do not vendor or patch Cage/wlroots in this repository.

## Acceptance

Repository checks and a temporary-prefix install pass. On an isolated VT, ten representative login/teardown cycles
produce no Cage core, greetd deactivation, or automatic restart. A normal reboot is then reviewed using the commands
in `docs/CAGE.md`; the boot ID, package revision, date, and results are recorded in `TASKS.md`.
