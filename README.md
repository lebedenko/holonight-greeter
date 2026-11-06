# HoloNight Greeter

For current build, test, container acceptance and provider workflows, see [DEVELOPMENT.md](DEVELOPMENT.md).

A Qt 6 / QML [greetd](https://sr.ht/~kennylevinsen/greetd/) greeter for the HoloNight desktop.

![HoloNight Greeter](docs/images/greeter.png)

## Features

- Fullscreen Wayland surfaces on every connected output, with a configurable primary output
- Local account discovery with UID, allowlist, denylist, locked-account, and avatar filtering
- Wayland session discovery and persisted last-user/last-session selection
- Configurable keyboard layouts and runtime layout switching under Hyprland
- Password, multi-step, and fingerprint authentication prompts through greetd
- Confirmed reboot and power-off actions through logind
- Windowed demo scenarios with simulated authentication and no privileged side effects
- Built-in isolated Hyprland launcher, with Cage available as an explicit rescue backend

## Requirements

Building requires a C++23 compiler, CMake 3.25 or newer, Ninja, Qt 6.11 (`Core`, `Gui`, `Quick`, `Qml`, `Network`,
`DBus`, and `Test` when tests are enabled), `layer-shell-qt`, toml++, HoloNight Qt 0.1.1 (`Core` and `Controls`),
and installed GTest >=1.14 CONFIG, Python 3, and the Qt QML private headers when tests are enabled.

Production additionally requires greetd, logind, and either Hyprland (the default) or Cage. Layer shell binds each
fullscreen greeter surface to its intended Wayland output.

## Build and test

See [DEVELOPMENT.md](DEVELOPMENT.md) for current native and container commands.

Application QML uses `QtQuick.Controls as Controls`. Each graphical executable
embeds a Holonight style default; `QT_QUICK_CONTROLS_STYLE=Fusion`, `-style Fusion`,
and an external `QT_QUICK_CONTROLS_CONF` remain supported. Core/composites retain
their HoloNight visuals. Installed QML discovery is relative to the executable.

## Demo

Run the greeter as a regular window from an existing graphical session:

```sh
task demo
task demo -- --demo-users single
task demo -- --demo-users multiple

# Other direct CLI scenarios after building:
./build/dev/holonight-greeter --demo
./build/dev/holonight-greeter --demo-scenario wrong-password
./build/dev/holonight-greeter --demo-scenario otp
./build/dev/holonight-greeter --demo-scenario fingerprint
```

`task demo` builds the selected `PRESET` (default `dev`) using installed packages, then opens the
`wrong-password` scenario. Every login attempt shows **Authentication failed**, with retries available.
Power actions show confirmations and simulated messages. Keyboard selection only updates the displayed
layout; it does not change the host keyboard or run `hyprctl`. No greetd connection, session start, state
write, or power-service call is made. For an additional installed development prefix, configure with
`cmake --preset dev -DCMAKE_PREFIX_PATH=/path/to/prefix` before running the task.

The optional `--demo-users single|multiple` replaces local account discovery with one or three fictional
accounts (Alice Morgan, Bob Chen, and Charlie Rivera), using the built-in fallback avatar. Fixture accounts
ignore configured user filters so their count is predictable. One account shows a centered plain name and skips the account selector during keyboard navigation;
multiple accounts enable a borderless dropdown and account switching. The keyboard selector sizes to its
current label, leaving the remaining footer width for the session selector. Icons use bundled outline SVGs. Omit the option to keep
local account discovery. Direct CLI use requires `--demo` or `--demo-scenario`; invalid profiles and use
outside demo mode are rejected. No system accounts are created or modified.

Supported scenarios are `default`, `wrong-password`, `otp`, and `fingerprint`; specifying a scenario implies
`--demo`. The demo uses configured read-only account and session discovery, including local display names and
avatars. Authentication, saved state, greetd communication, and logind actions are deterministic simulations.

On a compositor with `grim` and `slurp`, capture the demo with:

```sh
grim -g "$(slurp)" greeter.png
```

## Configuration

The sample [configuration](config/greeter.toml) covers:

- the `hyprland` or `cage` compositor backend and optional primary output;
- listed or manually entered users, UID bounds, filters, and avatars;
- session search directories, filters, and default session;
- keyboard layouts, variants, labels, and XKB options; and
- the background image path.

Configuration uses a strictly validated TOML v1 schema. Production defaults to `/etc/holonight/greeter.toml` and
accepts `--config PATH` and `--state PATH`. A missing configuration is nonfatal and uses compiled defaults; invalid
configuration disables authentication. Accounts without an assigned avatar use the fallback image bundled into the
executable. The system installation also provides optional faces under `/usr/share/pixmaps/faces`.

## System installation

Configure with the `/usr` prefix so GNUInstallDirs places machine configuration under `/etc`, then install and
provision the greeter state directory:

```sh
cmake -S . -B build-system -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-system
sudo cmake --install build-system
sudo systemd-tmpfiles --create holonight-greeter.conf
```

`task install:system` performs the same steps. Installation intentionally does not modify
`/etc/greetd/config.toml`. After preserving the administrator-owned configuration, merge the relevant values from
[the greetd example](deploy/greetd-config.toml). Its greeter command is:

```sh
/usr/bin/holonight-greeter-session --config /etc/holonight/greeter.toml
```

The launcher uses an isolated Hyprland instance by default and generates its private configuration under `/run`.
Select Cage in `greeter.toml` or override it explicitly with `--backend cage`. Startup failure is returned to greetd;
there is no automatic fallback between backends.

For a repository build on an isolated VT without replacing the installed greeter or primary greetd configuration,
run `task live:test`. See the [compositor deployment and recovery guide](docs/CAGE.md) before live testing or changing
the production login path.

## Documentation

- [MVP specification](docs/sdd/greeter-mvp/SPEC.md)
- [MVP design](docs/sdd/greeter-mvp/DESIGN.md)
- [Compositor deployment, live testing, and recovery](docs/CAGE.md)

## License

GPL-3.0-or-later — see [LICENSE](LICENSE).
