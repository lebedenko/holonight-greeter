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
    visible: greeterDemo
    title: "HoloNight Greeter"
    color: "#050b18"
    font.family: HolonightTheme.uiFont
    readonly property bool backgroundLoaded: wallpaper.status === Image.Ready
    readonly property bool backgroundLoadFailed: wallpaper.status === Image.Error
    signal backgroundReady
    signal backgroundFailed

    readonly property bool compact: width < 900
    readonly property real referenceScale: Math.min(width / 1672, height / 941)
    readonly property real panelScale: compact
                                       ? Math.min((width - 48) / 500, (height - 72) / 655)
                                       : Math.max(0.78, Math.min(1.25, referenceScale))
    property date now: new Date()
    property string pendingPowerAction: ""
    property Item pendingPowerOrigin: null

    function requestPowerAction(action, origin) {
        if (!greeterController.powerConfirmationRequired) {
            if (action === "reboot")
                greeterController.requestReboot(false)
            else
                greeterController.requestPowerOff(false)
            return
        }
        pendingPowerAction = action
        pendingPowerOrigin = origin
        Qt.callLater(function() {
            powerNoButton.forceActiveFocus(Qt.TabFocusReason)
        })
    }

    function cancelPowerConfirmation() {
        const origin = pendingPowerOrigin
        pendingPowerAction = ""
        pendingPowerOrigin = null
        if (origin)
            Qt.callLater(function() {
                origin.forceActiveFocus(Qt.TabFocusReason)
            })
    }

    function confirmPowerAction() {
        const action = pendingPowerAction
        pendingPowerAction = ""
        pendingPowerOrigin = null
        Qt.callLater(function() {
            if (action === "reboot")
                greeterController.requestReboot(true)
            else
                greeterController.requestPowerOff(true)
        })
    }

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
        id: wallpaper
        anchors.fill: parent
        source: greeterDemo ? "qrc:/qt/qml/Holonight/Greeter/assets/backgrounds/wallpaper.png"
                            : "file:" + greeterBackground
        fillMode: Image.PreserveAspectCrop
        onStatusChanged: {
            if (status === Image.Ready)
                root.backgroundReady()
            else if (status === Image.Error)
                root.backgroundFailed()
        }
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
        firstSystemAction: rebootAction.focusTarget
        lastSystemAction: powerAction.focusTarget
    }

    Shortcut {
        sequence: "Escape"
        enabled: root.pendingPowerAction.length === 0
                 && !["user-selection", "failed", "authenticated"].includes(greeterController.state)
        onActivated: greeterController.restartAuthentication()
    }

    Shortcut {
        sequence: "Escape"
        enabled: root.pendingPowerAction.length > 0
        onActivated: root.cancelPowerConfirmation()
    }

    Item {
        id: systemActions
        visible: !root.compact
        width: Math.max(normalPowerRow.implicitWidth, confirmationRow.implicitWidth)
        height: 62 * root.referenceScale
        anchors.right: parent.right
        anchors.rightMargin: 52 * root.referenceScale
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40 * root.referenceScale

        Row {
            id: normalPowerRow
            visible: root.pendingPowerAction.length === 0
            spacing: 28 * root.referenceScale

            SystemActionButton {
                id: rebootAction
                objectName: "rebootButton"
                symbol: "↻"
                description: greeterController.canReboot ? qsTr("Reboot") : qsTr("Reboot unavailable")
                enabled: greeterController.canReboot
                tabTarget: powerAction.focusTarget
                backtabTarget: loginPanel.footerFocusTarget
                onClicked: root.requestPowerAction("reboot", focusTarget)
            }
            SystemActionButton {
                id: powerAction
                objectName: "powerButton"
                symbol: "⏻"
                description: greeterController.canPowerOff ? qsTr("Shut down") : qsTr("Shutdown unavailable")
                enabled: greeterController.canPowerOff
                tabTarget: loginPanel.passwordFocusTarget
                backtabTarget: rebootAction.focusTarget
                onClicked: root.requestPowerAction("poweroff", focusTarget)
            }
        }

        Row {
            id: confirmationRow
            visible: root.pendingPowerAction.length > 0
            height: parent.height
            spacing: 10 * root.referenceScale

            Label {
                height: parent.height
                text: root.pendingPowerAction === "reboot" ? qsTr("Reboot?") : qsTr("Shut down?")
                color: HoloniightPalette.textPrimary
                verticalAlignment: Text.AlignVCenter
            }
            Button {
                id: powerYesButton
                text: qsTr("Yes")
                height: parent.height
                onClicked: root.confirmPowerAction()
                Keys.onSpacePressed: function(event) { event.accepted = true }
                KeyNavigation.tab: powerNoButton
                KeyNavigation.backtab: powerNoButton
            }
            Button {
                id: powerNoButton
                objectName: "powerConfirmationNoButton"
                text: qsTr("No")
                height: parent.height
                onClicked: root.cancelPowerConfirmation()
                Keys.onSpacePressed: function(event) { event.accepted = true }
                KeyNavigation.tab: powerYesButton
                KeyNavigation.backtab: powerYesButton
            }
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
                fillColor: !action.enabled ? HoloniightPalette.surfaceRaised
                           : action.down ? HoloniightPalette.surfaceElevated
                           : action.hovered ? HoloniightPalette.surfaceHover
                                            : HoloniightPalette.surface
                strokeColor: action.visualFocus ? HoloniightPalette.borderFocus
                             : action.down || action.hovered ? HoloniightPalette.borderActive
                                                            : HoloniightPalette.borderPassive
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
