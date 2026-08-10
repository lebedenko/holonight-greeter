// qmllint disable unqualified
import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import Holonight.Core
import Holonight.Controls

ApplicationWindow {
    id: root
    objectName: "greeterWindow"
    width: greeterDemo ? 1672 : Screen.width
    height: greeterDemo ? 941 : Screen.height
    visible: true
    visibility: greeterDemo ? Window.Windowed : Window.FullScreen
    title: "HoloNight Greeter"
    color: "#050b18"
    font.family: HolonightTheme.uiFont

    readonly property bool compact: width < 900
    readonly property real referenceScale: Math.min(width / 1672, height / 941)
    readonly property real panelScale: compact
                                       ? Math.min((width - 48) / 500, (height - 72) / 655)
                                       : Math.max(0.78, Math.min(1.25, referenceScale))
    property date now: new Date()

    onActiveChanged: {
        if (active)
            Qt.callLater(loginPanel.focusPassword)
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: root.now = new Date()
    }

    Image {
        anchors.fill: parent
        source: greeterDemo ? "qrc:/qt/qml/Holonight/Greeter/assets/backgrounds/wallpaper.png"
                            : "file:" + greeterBackground
        fillMode: Image.PreserveAspectCrop
    }

    Rectangle {
        anchors.fill: parent
        color: "#1a020914"
    }

    Item {
        id: clockBlock
        visible: !root.compact
        x: root.width * 0.06
        y: root.height * 0.37
        width: 430 * root.referenceScale
        height: 310 * root.referenceScale

        Column {
            spacing: 12 * root.referenceScale

            Label {
                text: Qt.formatTime(root.now, "hh:mm")
                color: "#f5f7ff"
                font.pointSize: 99 * root.referenceScale
                font.weight: Font.Light
                font.family: HolonightTheme.displayFont
            }
            Label {
                text: Qt.formatDate(root.now, "dddd, d MMMM")
                color: "#7697c5"
                font.pointSize: 22.5 * root.referenceScale
                font.weight: Font.Light
            }
            Rectangle {
                width: 50 * root.referenceScale
                height: 1
                color: "#668bb8"
                opacity: 0.8
            }
            Item { width: 1; height: 7 * root.referenceScale }
            Label {
                text: "HoloNight"
                color: "#f4f6ff"
                font.pointSize: 22.5 * root.referenceScale
                font.weight: Font.Medium
                font.family: HolonightTheme.titleFont
            }
            Label {
                text: greeterMachineName
                color: "#5b7da9"
                font.pointSize: 17.25 * root.referenceScale
            }
        }
    }

    LoginPanel {
        id: loginPanel
        width: 500
        height: 655
        scale: root.panelScale
        transformOrigin: Item.Center
        x: root.compact ? (root.width - width) / 2
                        : root.width - width - root.width * 0.104
        y: (root.height - height) / 2 - 6 * root.referenceScale
        firstSystemAction: accessibilityAction.focusTarget
        lastSystemAction: powerAction.focusTarget
    }

    Shortcut {
        sequence: "Escape"
        enabled: !["user-selection", "failed", "authenticated"].includes(greeterController.state)
        onActivated: greeterController.cancel()
    }

    Row {
        id: systemActions
        visible: !root.compact
        spacing: 28 * root.referenceScale
        anchors.right: parent.right
        anchors.rightMargin: 52 * root.referenceScale
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40 * root.referenceScale

        SystemActionButton {
            id: accessibilityAction
            objectName: "accessibilityButton"
            symbol: "♿︎"
            description: "Accessibility information"
            enabled: true
            tabTarget: rebootAction.focusTarget
            backtabTarget: loginPanel.footerFocusTarget
            onClicked: accessibilityDialog.open()
        }
        SystemActionButton {
            id: rebootAction
            objectName: "rebootButton"
            symbol: "↻"
            description: greeterController.canReboot ? "Reboot" : "Reboot unavailable"
            enabled: greeterController.canReboot
            tabTarget: powerAction.focusTarget
            backtabTarget: accessibilityAction.focusTarget
            onClicked: loginPanel.openRebootConfirmation()
        }
        SystemActionButton {
            id: powerAction
            objectName: "powerButton"
            symbol: "⏻"
            description: greeterController.canPowerOff ? "Shut down" : "Shutdown unavailable"
            enabled: greeterController.canPowerOff
            tabTarget: loginPanel.passwordFocusTarget
            backtabTarget: rebootAction.focusTarget
            onClicked: loginPanel.openPowerConfirmation()
        }
    }

    Dialog {
        id: accessibilityDialog
        anchors.centerIn: parent
        modal: true
        title: "Accessibility"
        standardButtons: Dialog.Close
        Label {
            text: "Keyboard navigation and visible focus are enabled.\nAdditional accessibility controls are planned."
            wrapMode: Text.Wrap
        }
    }

    Label {
        visible: !root.compact
        anchors.left: parent.left
        anchors.leftMargin: 56 * root.referenceScale
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 45 * root.referenceScale
        text: greeterController.selectedSessionName
        color: "#6584ad"
        font.pointSize: 15 * root.referenceScale
    }

    Label {
        visible: greeterConfigWarning.length > 0
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        text: greeterConfigWarning
        color: "#ffd479"
        wrapMode: Text.Wrap
    }

    component SystemActionButton: Item {
        id: actionFrame
        required property string symbol
        required property string description
        property Item tabTarget
        property Item backtabTarget
        readonly property alias focusTarget: action
        signal clicked()

        width: 62 * root.referenceScale
        height: 62 * root.referenceScale

        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                id: actionPath
                readonly property real chamfer: 10 * root.referenceScale
                readonly property real cornerRadius: 2 * root.referenceScale
                fillColor: "#a6081324"
                strokeColor: action.visualFocus ? "#80e6ff"
                                                : action.enabled ? "#7096c8" : "#3c5473"
                strokeWidth: action.visualFocus ? 2 : 1
                joinStyle: ShapePath.RoundJoin
                startX: chamfer
                startY: 0
                PathLine { x: actionFrame.width - actionPath.cornerRadius; y: 0 }
                PathQuad {
                    controlX: actionFrame.width; controlY: 0
                    x: actionFrame.width; y: actionPath.cornerRadius
                }
                PathLine { x: actionFrame.width; y: actionFrame.height - actionPath.chamfer }
                PathLine { x: actionFrame.width - actionPath.chamfer; y: actionFrame.height }
                PathLine { x: actionPath.cornerRadius; y: actionFrame.height }
                PathQuad {
                    controlX: 0; controlY: actionFrame.height
                    x: 0; y: actionFrame.height - actionPath.cornerRadius
                }
                PathLine { x: 0; y: actionPath.chamfer }
                PathLine { x: actionPath.chamfer; y: 0 }
            }
        }

        Button {
            id: action
            anchors.fill: parent
            enabled: actionFrame.enabled
            text: actionFrame.symbol
            font.pointSize: 23.25 * root.referenceScale
            Accessible.name: actionFrame.description
            ToolTip.visible: hovered
            ToolTip.text: actionFrame.description
            KeyNavigation.tab: actionFrame.tabTarget
            KeyNavigation.backtab: actionFrame.backtabTarget
            background: Item {}
            onClicked: actionFrame.clicked()
        }
    }
}
