# Greeter compositor deployment

Install the project with `task install:system`, or configure CMake with `-DCMAKE_INSTALL_PREFIX=/usr` before running
`sudo cmake --install`. The task also provisions the state directory through
`systemd-tmpfiles --create holonight-greeter.conf`. Do not use `cmake --install --prefix /usr` with a build configured
for another prefix: GNUInstallDirs resolves `/etc` layout during configuration. Then merge the documented example
into greetd configuration. The release launcher is:

```sh
/usr/bin/holonight-greeter-session --config /etc/holonight/greeter.toml
```

The release default is isolated Hyprland. Cage is a rescue backend selected in configuration or with `--backend
cage`; it uses the first configured XKB layout and does not expose runtime switching. There is deliberately no
automatic fallback. Recovery is to switch to another VT, restore the prior direct Cage greetd command, and restart
greetd. CMake installation never owns or modifies `/etc/greetd/config.toml`.

The explicit rescue invocation is:

```sh
/usr/bin/holonight-greeter-session --backend cage --config /etc/holonight/greeter.toml
```

Its built-in Cage output policy is `-m extend`, so every connected output receives a surface.

## Failure diagnosis

Keep the direct Cage command recorded as the rescue rollback. From
an isolated VT, run either `/bin/true` or the greeter as the child (interactive login and VT switching must be
performed manually):

```sh
/usr/libexec/holonight-greeter/holonight-greeter-cage-diagnostic /bin/true
/usr/libexec/holonight-greeter/holonight-greeter-cage-diagnostic holonight-greeter
```

The launcher records Cage's version, `WLR_DRM_DEVICES`, wlroots backend/output discovery, and Cage's exit status. It
leaves Cage stderr directly connected to the journal and emits no ANSI sequences itself. To isolate a hybrid-GPU
failure, set `WLR_DRM_DEVICES` to one resolved DRM device path per run; do not make that constraint permanent until a
single-device trigger is demonstrated.

Collect evidence without authentication payloads:

```sh
journalctl -b -u greetd --no-pager
journalctl -b /usr/bin/holonight-greeter --no-pager
coredumpctl info cage
coredumpctl list cage
```

From the repository, `task troubleshoot` runs the greetd journal, Cage coredump, and greetd service-status checks for
the last ten minutes. Run it after each manual login cycle.

For a repository build without changing the installed greeter or primary greetd configuration, run `task live:test`
from the existing session. It builds under `/tmp/holonight-greeter-live`, stages `greetd-live.toml`, and starts a
separate greetd instance with:

```sh
sudo greetd --config /tmp/holonight-greeter-live/greetd-live.toml \
  --socket-path /run/greetd-holonight-live.sock --vt 2
```

The temporary session uses the installed session launcher and `/etc/holonight/greeter.toml`. It temporarily stops
`getty@tty2`, restores
its prior active state on exit, enables compositor core dumps, and records foreground output in
`/tmp/holonight-greeter-live/greetd-live.log`. It does not stop or reconfigure the primary greetd service and does not
switch focus; switch to VT2 and interact with it manually.

Install matching Cage, wlroots, and Wayland debug symbols before capturing a backtrace. Compare packaged Cage 0.3.1
with `/bin/true` and the greeter, then a debug build of a pinned current wlroots-0.20-compatible upstream revision.
If upstream survives ten teardown cycles, package that exact commit with the Arch recipe and retain the official
package for rollback. If it still fails, identify the listener at `cage+0x92bd`, validate a minimal output-destroy
lifecycle patch under ASan/UBSan in a nested compositor, and package that patch against 0.3.1. Record the tested
package version and commit in the SDD; no revision has been accepted yet.

Write down rollback before replacing Cage. For an official package still present in pacman's cache, use:

```sh
sudo pacman -U /var/cache/pacman/pkg/cage-0.3.1-*.pkg.tar.zst
```

Success requires ten isolated-VT cycles without a Cage core or greetd restart, followed by one normal reboot. Record
the date, `cat /proc/sys/kernel/random/boot_id`, `cage -v`, package revision, and results from `journalctl -b`,
`journalctl -b -u greetd`, and `coredumpctl list cage` in the follow-up SDD.
