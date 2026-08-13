# HGR-104 Visual Refinement Design

`LoginPanel` defines one local footer selector presentation and instantiates it for both the controller session model
and the compositor layout model. Each instance owns half of the footer row, uses its control font for popup delegates,
and keeps its collapsed and popup row heights identical. Session selection continues through `Controller`;
keyboard selection uses the configured layout ID exposed by `CompositorAdapter`.
The shared `Holonight.ComboBox` owns overlay parenting and margins, so `FooterSelector` applies no geometry workaround.

`CompositorAdapter::selectLayout` resolves an ID only within the ordered configured list, invokes
`hyprctl switchxkblayout all <index>` only for Hyprland, and publishes the new ID and label only after a successful
command. `cycleLayout` delegates to this selection path and remains available for compatibility.

The controller treats greetd `auth_error` replies and authentication error prompts as one security boundary. It
discards their raw description and retains only the fixed user-facing status. Other protocol, transport, session,
configuration, and power errors retain their diagnostic text.

`IPowerService` publishes capabilities and a session-derived confirmation bit together. `LogindPowerService` queries
`ListSessions`, reads each session's `Class`, and requires confirmation when any class is `user` or any session query
fails. It subscribes to manager `SessionNew` and `SessionRemoved` signals and refreshes the combined state. Classes
such as `greeter` and `lock-screen` are naturally excluded because only exact `user` matches count.

The controller defaults the confirmation bit to true, rejects unconfirmed guarded requests, and otherwise gates only
on the corresponding capability. Demo mode keeps confirmation enabled and returns the existing simulated result.
`Main` swaps the normal action row for an inline confirmation row, manages initial/cancelled focus, and restores the
normal row before dispatching a confirmed request so backend errors appear against the stable layout.
