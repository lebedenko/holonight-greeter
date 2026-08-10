# HoloNight Greeter

A Qt 6 / QML greetd greeter for the HoloNight desktop.

```sh
task test
./build/holonight-greeter --demo
./build/holonight-greeter --demo-scenario wrong-password
./build/holonight-greeter --demo-scenario otp
./build/holonight-greeter --demo-scenario fingerprint
```

Production accepts `--config PATH` and `--state PATH`. Demo scenarios are `default`, `wrong-password`, `otp`, and
`fingerprint`; specifying a scenario implies `--demo`. Demo uses only deterministic synthetic data and is independent
of configuration, saved state, greetd, AccountsService, NSS, session discovery, and logind.

See [the MVP SDD](docs/sdd/greeter-mvp/SPEC.md) and [Cage deployment guide](docs/CAGE.md).
