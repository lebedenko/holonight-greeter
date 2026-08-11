# HoloNight Greeter

A Qt 6 / QML greetd greeter for the HoloNight desktop.

Build dependencies include Qt 6.11, `layer-shell-qt`, toml++, and the installed HoloNight Qt modules. Layer shell is
used to bind each fullscreen greeter surface to its intended Wayland output.

```sh
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build
cmake --install build
```

For a production system installation, set the `/usr` prefix while configuring so GNUInstallDirs places machine
configuration under `/etc` rather than `/usr/etc`:

```sh
cmake -S . -B build-system -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-system
sudo cmake --install build-system
sudo systemd-tmpfiles --create holonight-greeter.conf
```

`task install:system` performs those commands. It installs and provisions the greeter but intentionally does not edit
`/etc/greetd/config.toml`; merge the documented example separately after preserving the administrator configuration.

```sh
task test
./build/holonight-greeter --demo
./build/holonight-greeter --demo-scenario wrong-password
./build/holonight-greeter --demo-scenario otp
./build/holonight-greeter --demo-scenario fingerprint
```

Production accepts `--config PATH` and `--state PATH`. Demo scenarios are `default`, `wrong-password`, `otp`, and
`fingerprint`; specifying a scenario implies `--demo`. Demo uses the configured read-only account and session
discovery, including local display names and avatars. Authentication, saved state, greetd communication, and logind
actions remain deterministic simulations with no privileged side effects.

The install includes optional account faces under `/usr/share/pixmaps/faces`. Accounts without an assigned avatar use
the fallback image bundled into the greeter executable.

The installed greetd reference command is `/usr/bin/holonight-greeter-session --config
/etc/holonight/greeter.toml`. It selects only built-in adapters: Hyprland
by default, or Cage when configured or selected with `--backend cage`. Hyprland uses a generated private
configuration under `/run`; startup failure is returned to greetd and never triggers an automatic Cage fallback.

See [the MVP SDD](docs/sdd/greeter-mvp/SPEC.md) and [Cage deployment guide](docs/CAGE.md).
