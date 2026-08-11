# Boot and Login Reliability Design

Qt logging categories separate application lifecycle, controller lifecycle, and greetd protocol events. Log values
are fixed event names, enum-like state/type values, result codes, and presence flags. User-provided and
authentication-bearing values never cross the logging boundary.

`holonight-greeter-session` parses the typed greeter TOML and constructs fixed argument vectors for two compiled-in
adapters. `start-hyprland` receives a minimal private Lua configuration under `/run`; Cage receives fixed rescue
arguments.
Child stderr stays attached and failures propagate without fallback. `CompositorAdapter` exposes backend identity,
capabilities, and bounded fixed layout IPC. `OutputManager` owns passive wallpaper windows and moves the sole
interactive window using the deterministic selection policy whenever screens change.
The selection policy preserves compositor discovery order and never consults user configuration, EDID properties,
connector type, workspaces, or lexical ordering. All surfaces remain hidden until their screens are assigned and
every wallpaper reports ready; a wallpaper load failure terminates the greeter so greetd exposes the failure.
LayerShellQt gives each fullscreen window an output-bound layer-shell role before it is shown. The credential surface
uses the top layer with exclusive keyboard input; passive wallpaper surfaces use the background layer without input.

`holonight-greeter-cage-diagnostic` is installed under libexec for deliberate use from an isolated VT or a temporary
greetd command. It adds only plain-text markers, sets `WLR_LOG=debug` unless already selected, requests no-color
output, and runs Cage with the same extended-output arguments as the built-in rescue backend. It does not pipe Cage
output, so teardown status is retained and stderr reaches the service journal unchanged.

Cage ownership remains external. A symbolized reproducer determines whether to package a known-good pinned upstream
revision, a minimal listener-lifecycle patch, or temporarily constrain `WLR_DRM_DEVICES`. Package recipes, cores,
and upstream reports are operational artifacts and must not be vendored into the greeter.
