# Upstream report draft — transformed native TextInput caret disappears

Draft only; not submitted. No HoloNight dependency is needed by the reproduction.

## Environment and expected/actual behavior

Qt 6.11.2, Linux, Inter 14.25 pt. The native empty TextInput caret should remain
visible during its blink-on phase. At scale 0.78, DPR 1 and window 1276×1571,
its scene rectangle is `(771.616, 789.111, 0.78, 17.94)`, but the on/off images
have zero changed pixels. DPR 1.25, window 1021×1257, scene rectangle
`(543.136, 633.026, 0.78, 17.94)` produces 23 changed pixels in software.
UQC-217 also demonstrated both selected styles and private Sway/OpenGL.

The standalone source imports only QtQuick. It uses native TextInput and no
Controls, custom cursor, platform theme, greeter services or authentication.
Qt Test supplies event-loop waiting only. Inter must be installed: mismatched
font metrics fail the geometry assertion (exit 2), not the visibility assertion
(exit 1). Exit 0 means visibility passes. Native blinking is disabled only for
deterministic pixel subtraction; this does not implement a replacement cursor.

## Reproduction

From the greeter checkout:

```sh
cmake -S tests/diagnostics/qtquick-caret -B /tmp/qtquick-caret-build -G Ninja
cmake --build /tmp/qtquick-caret-build
python3 tests/diagnostics/qtquick-caret/run.py \
  /tmp/qtquick-caret-build/qtquick-caret --logs /tmp/qtquick-caret-software
python3 tests/diagnostics/qtquick-caret/run.py \
  /tmp/qtquick-caret-build/qtquick-caret --graphics --logs /tmp/qtquick-caret-opengl
```

Use fresh log directories. The graphics command creates its own headless Sway
compositor at output scale 1; it never interacts with the desktop compositor.
Both commands preserve visibility failure as a nonzero exit and write per-DPR
exit codes, exact geometry, full-window on/off and binary difference images.
Graphics requires Sway and a usable OpenGL driver.

## Matching source trace and interpretation

All source links below are pinned to `v6.11.2` in Qt's official repository.

1. [QQuickTextInput::cursorRectangle and updatePaintNode](https://github.com/qt/qtdeclarative/blob/v6.11.2/src/quick/items/qquicktextinput.cpp#L859)
   compute a width of one logical unit from text layout, padding and scrolling.
   `updatePaintNode` gates the native rectangle on focus/cursor visibility,
   read-only state and native blink status, then passes it to `setCursor`.
2. [QSGInternalTextNode::setCursor](https://github.com/qt/qtdeclarative/blob/v6.11.2/src/quick/items/qsginternaltextnode.cpp#L116)
   creates an internal rectangle through the scene-graph context, with the
   supplied rectangle and text color.
3. [QSGContext::createInternalRectangleNode](https://github.com/qt/qtdeclarative/blob/v6.11.2/src/quick/scenegraph/qsgcontext.cpp#L341)
   sets rectangle and color and updates the node; it does not enable antialiasing.
   [QSGBasicInternalRectangleNode](https://github.com/qt/qtdeclarative/blob/v6.11.2/src/quick/scenegraph/qsgbasicinternalrectanglenode.cpp#L78)
   defaults antialiasing to false and emits triangle geometry. The
   [vertex shader](https://github.com/qt/qtdeclarative/blob/v6.11.2/src/quick/scenegraph/shaders_ng/vertexcolor.vert)
   applies the transform matrix.
4. The [software node](https://github.com/qt/qtdeclarative/blob/v6.11.2/src/quick/scenegraph/adaptations/software/qsgsoftwareinternalrectanglenode.cpp#L32)
   aligns the local rectangle to integer bounds. Its `paintRectangle` disables
   QPainter antialiasing and fills the rectangle. Local integer alignment does
   not ensure coverage after scaling and translation.

**Demonstrated:** valid native local rectangle `(54,17,1,23)`, active focus,
cursor visibility true, exact mapped geometry and zero blink-on/off pixel delta
at DPR 1; visibility returns at DPR 1.25. Changing position/scale alone changes
coverage. Both render backends show this in UQC-217.

**Source-supported inference:** a thin, non-antialiased rectangle can fall
between pixel centres after transformation. DPR 1 horizontal interval
`[771.616, 772.396]` contains neither 771.5 nor 772.5. The DPR 1.25 interval
`[678.92, 679.895]` contains 679.5. This explains the measured contrast without
requiring a missing focus event or broken blink timer. This is not a claim that
every GPU/raster backend uses identical sampling rules; no instrumented Qt build
or upstream-maintainer confirmation has been performed.

## Bounded workaround evaluated and rejected

Greeter-only `layer.enabled: true; layer.smooth: true` restores caret coverage
in both styles, software/OpenGL, and DPR 1/1.25, including a quarter-physical-pixel
position sweep at scales 0.78, 1 and 1.25. Qt documents that the
[layer renders the subtree to a texture with linear filtering](https://doc.qt.io/qt-6/qml-qtquick-item.html#item-layers).
The same resampling visibly softens revealed text at scale 0.78. Under the local
requirement to preserve text sharpness, this candidate was rejected. No product
workaround is shipped; this report does not propose a custom cursor or offsets.

## Sanitized attachments

[Evidence directory](audit/uqc218/) contains full-window empty-field software
on/off/difference PNGs for both DPRs and unscaled crops of direct/layered text
from private OpenGL. The only populated text is disposable repeated `x` input.
No real username, password, session data or user profile is included. The local
UQC-218 record identifies the full evidence archive and verification outcome.
