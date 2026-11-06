// qmllint disable unqualified
import QtQuick
import QtQuick.Layouts
import Holonight.Controls
import Holonight.Core

HnIconComboBox {
    id: root

    required property url iconSource
    readonly property real naturalWidth: leftPadding + 24 + 10 + Math.ceil(labelMetrics.advanceWidth) + rightPadding

    TextMetrics {
        id: labelMetrics
        font: root.font
        text: root.displayText
    }

    iconRole: ""
    delegateHeight: height
    font.pointSize: 13.5
    hoverEnabled: true
    leftPadding: 14
    rightPadding: 44

    contentItem: RowLayout {
        spacing: 10

        HnIcon {
            source: root.iconSource
            size: 24
            iconState: root.enabled ? HnIcon.Normal : HnIcon.Disabled
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: root.displayText
            color: root.enabled ? HoloniightPalette.textPrimary : HoloniightPalette.textDisabled
            font: root.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    indicator: HnIcon {
        x: root.width - width - 14
        anchors.verticalCenter: parent.verticalCenter
        source: Qt.resolvedUrl("icons/chevron-down.svg")
        size: 24
        iconState: root.enabled ? HnIcon.Normal : HnIcon.Disabled
    }

    background: Rectangle {
        readonly property real semanticRadius:
            HnAppearance.roundedRadius(HnSurfaceRole.Control,
                                       width, height,
                                       HnAppearance.revision)

        radius: semanticRadius
        color: root.enabled && root.down ? HoloniightPalette.surfaceElevated
                         : root.enabled && root.HnInputInteraction.hoverAllowed && root.hovered ? HoloniightPalette.surfaceHover
                                        : "transparent"
        border.width: root.enabled && (root.visualFocus || root.popup.visible)
                      ? HnMetrics.focusBorderWidth : 0
        border.color: HoloniightPalette.borderFocus
    }
}
