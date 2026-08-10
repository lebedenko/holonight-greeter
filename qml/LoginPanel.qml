// qmllint disable unqualified
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import Qt5Compat.GraphicalEffects
import Holonight.Controls

Item {
    id: panel
    objectName: "loginPanel"
    property bool capsLockOn: false
    property Item firstSystemAction
    property Item lastSystemAction
    readonly property alias passwordFocusTarget: response
    readonly property alias footerFocusTarget: sessionSelector
    readonly property var currentUser: userSelector.currentIndex >= 0
                                       ? greeterController.users[userSelector.currentIndex]
                                       : null
    readonly property string selectedAvatar: currentUser && currentUser.avatar
                                             ? String(currentUser.avatar) : ""
    property string selectedUser: greeterController.manualMode ? username.text :
                                  (userSelector.currentIndex >= 0 ? userSelector.currentValue : "")

    function openRebootConfirmation() { rebootDialog.open() }
    function openPowerConfirmation() { powerDialog.open() }
    function focusPassword() {
        if (response.visible && response.enabled)
            response.forceActiveFocus(Qt.TabFocusReason)
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            fillColor: "#e6081222"
            strokeColor: "#6a91bf"
            strokeWidth: 1
            joinStyle: ShapePath.RoundJoin
            PathSvg {
                path: "M 14 0 L 435 0 L 500 65 L 500 641 L 486 655 L 65 655 L 0 590 L 0 14 Z"
            }
        }
    }

    Component.onCompleted: {
        if (greeterDemo && panel.selectedUser.length > 0) {
            greeterController.begin(panel.selectedUser)
            Qt.callLater(panel.focusPassword)
        }
    }

    Connections {
        target: greeterController
        function onChanged() {
            if (greeterController.state === "input-prompt")
                Qt.callLater(panel.focusPassword)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 38
        anchors.leftMargin: 40
        anchors.rightMargin: 40
        anchors.bottomMargin: 22
        spacing: 0

        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 132
            Layout.preferredHeight: 132

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: "#0b1729"
                border.color: "#7398c5"
                border.width: 1
            }
            Rectangle {
                anchors.fill: parent
                anchors.margins: 4
                radius: width / 2
                color: "#0b1729"
                visible: panel.selectedAvatar.length > 0
                Image {
                    id: avatarImage
                    anchors.fill: parent
                    source: "file:" + panel.selectedAvatar
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    visible: false
                }
                OpacityMask {
                    anchors.fill: parent
                    source: avatarImage
                    maskSource: Rectangle {
                        width: avatarImage.width
                        height: avatarImage.height
                        radius: width / 2
                    }
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: 21
                width: 48
                height: 54
                radius: 24
                color: "#050c19"
                visible: panel.selectedAvatar.length === 0
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 8
                width: 96
                height: 58
                radius: 48
                color: "#050c19"
                visible: panel.selectedAvatar.length === 0
            }
        }

        ComboBox {
            id: userSelector
            objectName: "userSelector"
            Layout.topMargin: 16
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 250
            model: greeterController.users
            textRole: "displayName"
            valueRole: "username"
            enabled: count > 1 && !["starting", "authenticated"].includes(greeterController.state)
            font.pointSize: 22.5
            contentItem: Label {
                text: userSelector.displayText
                color: "#f3f5fc"
                font: userSelector.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            indicator: Item {}
            background: Item {}
            onPressedChanged: {
                if (pressed && greeterController.state !== "user-selection") {
                    greeterController.cancel()
                    Qt.callLater(popup.open)
                }
            }
            onActivated: {
                if (greeterController.state === "user-selection")
                    greeterController.begin(currentValue)
            }
            KeyNavigation.tab: response.visible ? response : primary
            KeyNavigation.backtab: panel.lastSystemAction
        }

        TextField {
            id: username
            objectName: "usernameField"
            visible: greeterController.manualMode
            Layout.topMargin: 16
            Layout.fillWidth: true
            placeholderText: "Username"
            enabled: greeterConfigError.length === 0 && greeterController.state === "user-selection"
            KeyNavigation.tab: primary
        }

        Label {
            visible: !greeterController.manualMode
            Layout.alignment: Qt.AlignHCenter
            text: "Local account"
            color: "#5e7da7"
            font.pointSize: 15
        }

        Item { Layout.preferredHeight: 34 }

        Label {
            visible: response.visible
            text: greeterController.prompt
            color: "#7194c1"
            font.pointSize: 12
        }

        Item {
            visible: response.visible
            Layout.fillWidth: true
            Layout.preferredHeight: 57
            Layout.topMargin: 8

            TextField {
                id: response
                objectName: "responseField"
                anchors.fill: parent
                leftPadding: 54
                rightPadding: greeterController.secret ? 54 : 14
                font.pointSize: 14.25
                echoMode: reveal.pressed || !greeterController.secret ? TextInput.Normal : TextInput.Password
                onAccepted: {
                    greeterController.respond(text)
                    text = ""
                }
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_CapsLock)
                        panel.capsLockOn = !panel.capsLockOn
                }
                onVisibleChanged: {
                    if (!visible)
                        text = ""
                    else
                        Qt.callLater(panel.focusPassword)
                }
                KeyNavigation.tab: reveal.visible ? reveal : primary
                KeyNavigation.backtab: panel.lastSystemAction
            }
            Label {
                anchors.left: parent.left
                anchors.leftMargin: 17
                anchors.verticalCenter: parent.verticalCenter
                text: "♙"
                color: "#83a8d3"
                font.pointSize: 18.75
            }
            Button {
                id: reveal
                objectName: "revealButton"
                visible: greeterController.secret
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 44
                height: 44
                text: pressed ? "◉" : "◎"
                Accessible.name: "Hold to reveal password"
                background: Item {}
                KeyNavigation.tab: primary
                KeyNavigation.backtab: response
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            Layout.topMargin: 12
            text: response.visible && greeterController.secret && panel.capsLockOn
                  ? "⚠  Caps Lock is on"
                  : greeterController.state === "failed" ? greeterController.status : ""
            color: greeterController.state === "failed" ? "#ff89a2" : "#bb7cec"
            font.pointSize: 12
            elide: Text.ElideRight
        }

        Label {
            visible: greeterController.state === "informational-prompt"
            Layout.fillWidth: true
            text: greeterController.prompt
            color: "#dce6f5"
            font.pointSize: 13.5
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Label {
            visible: greeterConfigError.length > 0
            Layout.fillWidth: true
            text: "Configuration error\n" + greeterConfigError
            color: "#ff718c"
            wrapMode: Text.Wrap
        }

        Item { Layout.fillHeight: true; Layout.minimumHeight: 18 }

        Button {
            id: primary
            objectName: "primaryButton"
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            text: greeterController.state === "failed" ? "Retry" :
                  response.visible ? "Log in" : "Continue"
            enabled: greeterConfigError.length === 0 && sessionSelector.count > 0
                     && ["user-selection", "failed", "input-prompt"].includes(greeterController.state)
            onClicked: {
                if (response.visible) {
                    greeterController.respond(response.text)
                    response.text = ""
                } else {
                    greeterController.begin(panel.selectedUser)
                }
            }
            KeyNavigation.tab: sessionSelector
            KeyNavigation.backtab: reveal.visible ? reveal : response
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 28
            Layout.preferredHeight: 1
            color: "#30435e"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            spacing: 14

            Label { text: "▱"; color: "#88a9d0"; font.pointSize: 20.25 }
            ComboBox {
                id: sessionSelector
                objectName: "sessionSelector"
                Layout.fillWidth: true
                model: greeterController.sessions
                textRole: "name"
                valueRole: "id"
                enabled: greeterConfigError.length === 0 && count > 0
                         && !["starting", "authenticated"].includes(greeterController.state)
                background: Item {}
                Component.onCompleted: {
                    const wanted = indexOfValue(greeterController.selectedSession)
                    if (wanted >= 0)
                        currentIndex = wanted
                }
                onActivated: greeterController.selectedSession = currentValue
                KeyNavigation.tab: panel.firstSystemAction
                KeyNavigation.backtab: primary
            }
            Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 38; color: "#30435e" }
            Label { text: "⌨"; color: "#88a9d0"; font.pointSize: 16.5 }
            Label {
                objectName: "keyboardSelector"
                Layout.preferredWidth: 100
                text: greeterKeyboardLabel
                color: "#6884aa"
                horizontalAlignment: Text.AlignHCenter
                Accessible.description: "Keyboard layout is configured by the administrator"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#30435e"
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 18
            text: greeterController.status.length > 0 ? greeterController.status
                  : greeterController.state === "connecting" ? "Connecting to authentication service"
                  : greeterController.state === "waiting" ? "Waiting for authentication"
                  : greeterController.state === "starting" ? "Starting selected session"
                  : greeterController.state === "authenticated" ? "Authenticated"
                  : "Ready to authenticate"
            color: greeterController.state === "failed" ? "#ff89a2" : "#526b8e"
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }

    Dialog {
        id: rebootDialog
        title: "Reboot this computer?"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        onAccepted: greeterController.requestReboot()
    }
    Dialog {
        id: powerDialog
        title: "Shut down this computer?"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        onAccepted: greeterController.requestPowerOff()
    }
}
