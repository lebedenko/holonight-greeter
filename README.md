# HoloNight Greeter

A Qt 6 / QML greetd greeter for the HoloNight desktop.

```sh
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build
cmake --install build --prefix ~/.local
```

```sh
task test
./build/holonight-greeter --demo
./build/holonight-greeter --demo-scenario wrong-password
./build/holonight-greeter --demo-scenario otp
./build/holonight-greeter --demo-scenario fingerprint
```

Production accepts `--config PATH` and `--state PATH`. Demo scenarios are `default`, `wrong-password`, `otp`, and
`fingerprint`; specifying a scenario implies `--demo`. Demo keeps authentication deterministic and is independent of
configuration, saved state, greetd, AccountsService, and logind, while still showing the local identity/avatar and
discovering the machine's installed Wayland sessions.

See [the MVP SDD](docs/sdd/greeter-mvp/SPEC.md) and [Cage deployment guide](docs/CAGE.md).
