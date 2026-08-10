# HoloNight Greeter

A Qt 6 / QML greetd greeter for the HoloNight desktop.

```sh
task test
./build/holonight-greeter --demo --demo-scenario default
```

Production accepts `--config PATH` and `--state PATH`. Demo scenarios are `default`, `wrong-password`, `otp`, and
`fingerprint`; demo is independent of greetd, AccountsService, and logind.

See [the MVP SDD](docs/sdd/greeter-mvp/SPEC.md) and [Cage deployment guide](docs/CAGE.md).
