# HoloNight Greeter MVP Design

## Structure

`greeter_core` is the sole internal C++ library. Value-oriented configuration, desktop-entry, and state services are
isolated from Qt Quick. `GreetdClient` owns framed socket I/O; `GreeterController` is the narrow QML state machine.
Account, power, clock, transport, and filesystem boundaries are injectable so automated tests use fakes and temporary
directories only.

The executable selects production or demo behavior, exposes one controller instance, and loads the `Holonight.Greeter`
QML module. Production uses a fullscreen window; demo is windowed, discovers accounts and sessions through the same
read-only adapters, and synthesizes prompts and system actions locally.

## Data flow

Configuration is loaded first. A fatal result reaches QML as a configuration-error state and no greetd connection is
created. Eligible users and sessions are stable-sorted. Session selection uses valid persisted state, configured
default, then the first session. Successful `start_session` acknowledgement is the only state-write trigger.

The greetd transport consumes native-endian uint32 lengths and incremental JSON payloads with a 1 MiB hard limit.
Controller actions are gated by protocol state. Cancellation sends `cancel_session` whenever connected.

## Deployment

CMake installs the executable, compiled QML/resources, optional system account faces, default configuration, tmpfiles
rule, Cage guide, and a reference greetd configuration. The no-avatar fallback remains a binary resource instead of a
system face. The reference greetd file is documentation, never `/etc/greetd/config.toml`. Distribution packaging is
deferred until release planning.
