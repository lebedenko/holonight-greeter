// qmllint disable unqualified
import QtQuick
import QtQuick.Layouts
import Holonight.Controls
import Holonight.Core

HnIconComboBox {
    id: root

    required property string iconText

    iconRole: ""
    delegateHeight: height
    font.pointSize: 13.5
    hoverEnabled: true
    leftPadding: 14
    rightPadding: 34

    contentItem: RowLayout {
        spacing: 10

        Text {
            text: root.iconText
            color: root.enabled ? HoloniightPalette.textSecondary : HoloniightPalette.textDisabled
            font.pointSize: root.iconText === "⌨" ? 16.5 : 20.25
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

    indicator: Text {
        x: root.width - width - 14
        anchors.verticalCenter: parent.verticalCenter
        text: "⌄"
        color: root.enabled ? HoloniightPalette.textSecondary : HoloniightPalette.textDisabled
        font: root.font
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
