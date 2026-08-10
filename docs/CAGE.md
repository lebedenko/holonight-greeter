# Cage deployment

Install the package, provision the state directory with `systemd-tmpfiles --create holonight-greeter.conf`, then merge
the documented example into greetd configuration. The tested launcher shape is:

```sh
XKB_DEFAULT_LAYOUT=us XKB_DEFAULT_VARIANT= XKB_DEFAULT_OPTIONS= dbus-run-session cage -s -m last -d -- holonight-greeter
```

Set matching `XKB_DEFAULT_*` values in greetd's service environment. The `EN` label is static in this MVP; runtime
layout switching is not implemented. Recovery is to switch to another VT, restore the prior greetd configuration,
restart greetd, and uninstall the package. No package file owns `/etc/greetd/config.toml`.
