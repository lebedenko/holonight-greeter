// qmllint disable unqualified
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Holonight.Controls

HnSurfaceFrame {
    id: panel
    property bool capsLockOn: false
    property string selectedUser: greeterController.manualMode ? "" :
                                  (userSelector.currentIndex >= 0 ? userSelector.currentValue : "")
    fillColor: "#e8111520"
    borderColor: "#507ee8ff"
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 40; spacing: 18
        Label { text: "SIGN IN"; font.pixelSize: 13; font.letterSpacing: 3; color: "#80e6ff" }
        Item { Layout.fillHeight: true }
        Rectangle {
            Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 88; Layout.preferredHeight: 88; radius: 44; color: "#263146"
            Label { anchors.centerIn: parent; text: "HN"; font.pixelSize: 26; color: "white" }
        }
        ComboBox {
            id: userSelector; objectName: "userSelector"; Layout.fillWidth: true
            visible: !greeterController.manualMode
            model: greeterController.users
            textRole: "displayName"; valueRole: "username"
            enabled: greeterConfigError.length === 0 && greeterController.state === "user-selection"
            KeyNavigation.tab: sessionSelector
        }
        TextField {
            id: username; objectName: "usernameField"; Layout.fillWidth: true; placeholderText: "Username"
            visible: greeterController.manualMode
            enabled: greeterConfigError.length === 0 && greeterController.state === "user-selection"
            KeyNavigation.tab: sessionSelector
        }
        Label { visible: greeterConfigError.length > 0; text: "Configuration error\n" + greeterConfigError; wrapMode: Text.Wrap; color: "#ff718c"; Layout.fillWidth: true }
        TextField {
            id: response; objectName: "responseField"; Layout.fillWidth: true
            visible: greeterController.state === "input-prompt"
            placeholderText: greeterController.prompt
            echoMode: reveal.pressed || !greeterController.secret ? TextInput.Normal : TextInput.Password
            onAccepted: { greeterController.respond(text); text = "" }
            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_CapsLock) panel.capsLockOn = !panel.capsLockOn
            }
            onVisibleChanged: if (!visible) text = ""
            KeyNavigation.tab: reveal
        }
        Label { visible: response.visible && greeterController.secret && panel.capsLockOn; text: "Caps Lock is on"; color: "#ffd479" }
        Button { id: reveal; objectName: "revealButton"; visible: response.visible && greeterController.secret; text: pressed ? "Release to hide" : "Hold to reveal"; KeyNavigation.tab: primary }
        Label { visible: greeterController.state === "informational-prompt"; text: greeterController.prompt; color: "white"; wrapMode: Text.Wrap }
        ComboBox {
            id: sessionSelector; objectName: "sessionSelector"; Layout.fillWidth: true
            model: greeterController.sessions; textRole: "name"; valueRole: "id"
            enabled: greeterConfigError.length === 0 && count > 0 && greeterController.state === "user-selection"
            Component.onCompleted: {
                const wanted = indexOfValue(greeterController.selectedSession)
                if (wanted >= 0) currentIndex = wanted
            }
            onActivated: greeterController.selectedSession = currentValue
            KeyNavigation.tab: primary
        }
        RowLayout {
            Layout.fillWidth: true
            Label { text: greeterKeyboardLabel; color: "#ccd1dd" }
            Item { Layout.fillWidth: true }
            Label { text: greeterController.status; color: "#ff9aad" }
        }
        Button {
            id: primary
            objectName: "primaryButton"; Layout.fillWidth: true
            text: greeterController.state === "failed" ? "Retry" : response.visible ? "Continue" : "Sign in"
            enabled: greeterConfigError.length === 0 && sessionSelector.count > 0
                     && ["user-selection", "failed", "input-prompt"].includes(greeterController.state)
            onClicked: {
                if (response.visible) { greeterController.respond(response.text); response.text = "" }
                else if (greeterController.state === "failed") greeterController.begin(panel.selectedUser || username.text)
                else greeterController.begin(panel.selectedUser || username.text)
            }
            KeyNavigation.tab: cancelButton.visible ? cancelButton : rebootButton
        }
        Button {
            id: cancelButton; objectName: "cancelButton"; Layout.fillWidth: true; text: "Cancel"
            visible: !["user-selection", "failed", "authenticated"].includes(greeterController.state)
            onClicked: { response.text = ""; greeterController.cancel() }
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Button { id: rebootButton; objectName: "rebootButton"; text: "Reboot"; enabled: greeterController.canReboot; onClicked: rebootDialog.open(); ToolTip.visible: hovered && !enabled; ToolTip.text: "Unavailable without logind authorization" }
            Button { id: powerButton; objectName: "powerButton"; text: "Shut down"; enabled: greeterController.canPowerOff; onClicked: powerDialog.open(); ToolTip.visible: hovered && !enabled; ToolTip.text: "Unavailable without logind authorization" }
        }
        Item { Layout.fillHeight: true }
    }
    Dialog {
        id: rebootDialog; title: "Reboot this computer?"; modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        onAccepted: greeterController.requestReboot()
    }
    Dialog {
        id: powerDialog; title: "Shut down this computer?"; modal: true; anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        onAccepted: greeterController.requestPowerOff()
    }
}
