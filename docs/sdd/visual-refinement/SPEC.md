# HGR-104 Visual Refinement Specification

## Problem

The footer exposes session and keyboard choices with unrelated presentations, authentication failures can leak
backend wording, and power actions always use modal confirmation without considering logged-in sessions. System
actions also lack complete pointer feedback and include a non-functional accessibility dialog.

## Requirements

- Present session and keyboard layout as equal-width, 66 px footer selectors with a shared icon, text, chevron,
  focus, hover, and popup-delegate treatment. Popup rows inherit the control font and height.
- Consume `HolonightQt 0.1.1` popup geometry directly without consumer popup-parent or margin overrides.
- Select any configured keyboard layout by its ordered model index through Hyprland IPC. Invalid identifiers,
  unsupported backends, and IPC failures must preserve the displayed selection. Retain layout cycling compatibility.
- Connect footer keyboard navigation directly through Reboot and Shut down and back to the login controls. Remove
  the accessibility action and dialog.
- Show exactly `Authentication failed` for greetd authentication rejection text and error prompts. Preserve detailed
  errors that are not authentication rejections.
- Give enabled Reboot and Shut down actions distinct hover, pressed, and keyboard-focus states using HoloNight
  surface and border palette tokens. Preserve their disabled state.
- Require power confirmation conservatively until logind proves no session whose class is `user` exists. Include
  local, remote, foreground, and inactive user sessions; exclude greeter and lock-screen session classes. Refresh
  after session creation/removal and require confirmation after query failure.
- Keep capability as the only power-action enablement gate. Authentication in progress must not suppress an enabled
  power request.
- Execute enabled actions immediately when confirmation is unnecessary. Otherwise replace the action icons inline
  with the action question and explicit Yes/No controls. Focus No initially; No or Escape cancels and restores the
  originating action; Yes restores the normal row before requesting the action.
- Require an explicit confirmed flag at the controller boundary for guarded requests. Demo mode performs no logind
  calls, simulates actions, and defaults to requiring confirmation.

## Acceptance

Automated tests cover authentication normalization, power gating and confirmation behavior, and arbitrary layout
selection with failure preservation. The build, CTest, formatting, qmllint, QML compilation, demo smoke scenarios,
and `git diff --check` pass. Manual demo inspection and isolated-VT session-aware power testing remain explicitly
tracked and must not disrupt an unrelated active session.
