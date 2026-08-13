// qmllint disable unqualified
import QtQuick
import QtQuick.Layouts
import Holonight as Hn
import Holonight.Core

Hn.ComboBox {
    id: root

    required property string iconText

    delegateHeight: height
    font.pointSize: 13.5
    hoverEnabled: true
    leftPadding: 14
    rightPadding: 34

    contentItem: RowLayout {
        spacing: 10

        Text {
            text: root.iconText
            color: "#88a9d0"
            font.pointSize: root.iconText === "⌨" ? 16.5 : 20.25
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: root.displayText
            color: root.enabled ? "#dce6f5" : "#6884aa"
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
        color: root.enabled ? "#88a9d0" : "#526b8e"
        font: root.font
    }

    background: Rectangle {
        readonly property real semanticRadius:
            HnAppearance.roundedRadius(HnSurfaceRole.Control,
                                       width, height,
                                       HnAppearance.revision)

        radius: semanticRadius
        color: root.down ? HoloniightPalette.surfaceElevated
                         : root.hovered ? HoloniightPalette.surfaceHover
                                        : "transparent"
        border.width: root.visualFocus || root.popup.visible
                      ? HnMetrics.focusBorderWidth : 0
        border.color: HoloniightPalette.borderFocus
    }
}
