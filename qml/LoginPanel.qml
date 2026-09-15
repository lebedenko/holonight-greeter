// qmllint disable unqualified
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Shapes
import Holonight.Core
import Holonight.Controls

Item {
    id: panel
    objectName: "loginPanel"
    property bool capsLockOn: false
    property Item firstSystemAction
    property Item lastSystemAction
    function nextFocus(current, direction) {
        const cycle = [userSelector, username, response, reveal, primary,
                       sessionSelector, keyboardSelector, firstSystemAction, lastSystemAction]
        const start = cycle.indexOf(current)
        for (let step = 1; step <= cycle.length; ++step) {
            const candidate = cycle[(start + direction * step + cycle.length) % cycle.length]
            if (candidate && candidate.visible && candidate.enabled)
                return candidate
        }
        return null
    }
    readonly property var currentUser: userSelector.currentIndex >= 0
                                       ? greeterController.users[userSelector.currentIndex]
                                       : null
    readonly property string selectedAvatar: currentUser && currentUser.avatar
                                             ? String(currentUser.avatar) : ""
    property string selectedUser: greeterController.manualMode ? username.text :
                                  (userSelector.currentIndex >= 0 ? userSelector.currentValue : "")

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
        if (greeterConfigError.length === 0 && !greeterController.manualMode
                && greeterController.initialUser.length > 0) {
            const wanted = userSelector.indexOfValue(greeterController.initialUser)
            if (wanted >= 0)
                userSelector.currentIndex = wanted
            greeterController.begin(panel.selectedUser)
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

        HnAvatar {
            objectName: "userAvatar"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 132
            Layout.preferredHeight: 132
            size: 132
            imageInset: 4
            backgroundColor: "#0b1729"
            ringColor: "#7398c5"
            ringWidth: 1
            source: panel.selectedAvatar.length > 0 ? "file:" + panel.selectedAvatar : ""
            fallbackSource: Qt.resolvedUrl("images/no-avatar.png")
        }

        HnIconComboBox {
            id: userSelector
            objectName: "userSelector"
            Layout.topMargin: 16
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 250
            Layout.preferredHeight: 48
            visible: !greeterController.manualMode
            iconRole: ""
            delegateHeight: 56
            model: greeterController.users
            textRole: "displayName"
            valueRole: "username"
            enabled: count > 1 && !["starting", "authenticated"].includes(greeterController.state)
            font.pointSize: 22.5
            contentItem: Controls.Label {
                text: userSelector.displayText
                color: userSelector.enabled ? HoloniightPalette.textPrimary : HoloniightPalette.textDisabled
                elide: Text.ElideRight
                font: userSelector.font
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            delegate: Controls.ItemDelegate {
                id: accountRow
                required property int index
                readonly property string avatarPath: userSelector.roleValue(index, "avatar") || ""
                objectName: "accountRow" + index
                width: userSelector.popup.availableWidth
                height: userSelector.delegateHeight
                text: userSelector.textAt(index)
                highlighted: userSelector.highlightedIndex === index
                hoverEnabled: HnInputInteraction.hoverAllowed
                palette: userSelector.palette
                contentItem: RowLayout {
                    spacing: 10
                    HnAvatar {
                        objectName: "accountAvatar"
                        size: 40
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        source: accountRow.avatarPath ? "file:" + accountRow.avatarPath : ""
                        fallbackSource: Qt.resolvedUrl("images/no-avatar.png")
                    }
                    Controls.Label {
                        Layout.fillWidth: true
                        text: accountRow.text
                        elide: Text.ElideRight
                        color: accountRow.palette.buttonText
                    }
                }
            }
            onActivated: greeterController.begin(currentValue)
            KeyNavigation.tab: panel.nextFocus(userSelector, 1)
            KeyNavigation.backtab: panel.nextFocus(userSelector, -1)
        }

        Controls.TextField {
            id: username
            objectName: "usernameField"
            visible: greeterController.manualMode
            Layout.topMargin: 16
            Layout.fillWidth: true
            placeholderText: "Username"
            enabled: greeterConfigError.length === 0 && greeterController.state === "user-selection"
            onAccepted: greeterController.begin(text)
            KeyNavigation.tab: panel.nextFocus(username, 1)
            KeyNavigation.backtab: panel.nextFocus(username, -1)
        }

        Controls.Label {
            visible: !greeterController.manualMode
            Layout.alignment: Qt.AlignHCenter
            text: "Local account"
            color: "#5e7da7"
            font.pointSize: 15
        }

        Item { Layout.preferredHeight: 34 }

        Controls.Label {
            visible: response.visible
            text: greeterController.prompt
            color: "#7194c1"
            font.pointSize: 12
        }

        Item {
            visible: greeterController.state === "input-prompt"
            Layout.fillWidth: true
            Layout.preferredHeight: 57
            Layout.topMargin: 8

            Controls.TextField {
                id: response
                objectName: "responseField"
                anchors.fill: parent
                leftPadding: 54
                rightPadding: greeterController.secret ? 54 : 14
                font.pointSize: 14.25
                echoMode: greeterController.secret && !reveal.held
                          ? TextInput.Password : TextInput.Normal
                onAccepted: {
                    greeterController.respond(text)
                    text = ""
                }
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_CapsLock) {
                        panel.capsLockOn = !panel.capsLockOn
                    }
                }
                Keys.priority: Keys.BeforeItem
                onVisibleChanged: {
                    if (!visible)
                        text = ""
                    else
                        Qt.callLater(panel.focusPassword)
                }
                KeyNavigation.tab: panel.nextFocus(response, 1)
                KeyNavigation.backtab: panel.nextFocus(response, -1)
                KeyNavigation.priority: KeyNavigation.BeforeItem
            }
            Controls.Label {
                anchors.left: parent.left
                anchors.leftMargin: 17
                anchors.verticalCenter: parent.verticalCenter
                text: "♙"
                color: "#83a8d3"
                font.pointSize: 18.75
            }
            Controls.Button {
                id: reveal
                objectName: "revealButton"
                visible: greeterController.secret
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 44
                height: 44
                property bool held: false
                text: held ? "◉" : "◎"
                onPressed: held = true
                onReleased: held = false
                onCanceled: held = false
                onActiveFocusChanged: if (!activeFocus) held = false
                onVisibleChanged: if (!visible) held = false
                onEnabledChanged: if (!enabled) held = false
                Keys.priority: Keys.BeforeItem
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Space) {
                        if (!event.isAutoRepeat)
                            held = true
                        event.accepted = true
                    } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        event.accepted = true
                    }
                }
                Keys.onReleased: function(event) {
                    if (event.key === Qt.Key_Space) {
                        if (!event.isAutoRepeat)
                            held = false
                        event.accepted = true
                    }
                }
                Connections {
                    target: reveal.Window.window
                    function onActiveChanged() {
                        if (!reveal.Window.window.active)
                            reveal.held = false
                    }
                }
                Accessible.name: "Hold to reveal password"
                background: Rectangle {
                    readonly property real semanticRadius:
                        HnAppearance.roundedRadius(HnSurfaceRole.Control,
                                                   width, height,
                                                   HnAppearance.revision)
                    radius: semanticRadius
                    color: reveal.enabled && reveal.held ? HoloniightPalette.surfaceElevated
                                       : reveal.enabled && reveal.HnInputInteraction.hoverAllowed && reveal.hovered ? HoloniightPalette.surfaceHover
                                                        : "transparent"
                    border.width: reveal.enabled && reveal.visualFocus ? HnMetrics.focusBorderWidth : 0
                    border.color: HoloniightPalette.borderFocus
                }
                KeyNavigation.tab: panel.nextFocus(reveal, 1)
                KeyNavigation.backtab: panel.nextFocus(reveal, -1)
            }
        }

        Controls.Label {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            Layout.topMargin: 12
            text: response.visible && greeterController.secret && panel.capsLockOn
                  ? "⚠  Caps Lock is on" : ""
            color: "#bb7cec"
            font.pointSize: 12
            elide: Text.ElideRight
        }

        Controls.Label {
            visible: greeterController.state === "informational-prompt"
            Layout.fillWidth: true
            text: greeterController.prompt
            color: "#dce6f5"
            font.pointSize: 13.5
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
        }

        Controls.Label {
            visible: greeterConfigError.length > 0
            Layout.fillWidth: true
            text: "Configuration error\n" + greeterConfigError
            color: "#ff718c"
            wrapMode: Text.Wrap
        }

        Item { Layout.fillHeight: true; Layout.minimumHeight: 18 }

        Controls.Button {
            id: primary
            objectName: "primaryButton"
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            visible: greeterController.state === "input-prompt"
                     || greeterController.state === "failed"
                     || (greeterController.manualMode
                         && greeterController.state === "user-selection")
            text: "Log in"
            enabled: visible && greeterConfigError.length === 0
                     && sessionSelector.count > 0
                     && (!greeterController.manualMode
                         || greeterController.state !== "user-selection"
                         || username.text.trim().length > 0)
            onClicked: {
                if (response.visible) {
                    greeterController.respond(response.text)
                    response.text = ""
                } else {
                    greeterController.begin(panel.selectedUser)
                }
            }
            KeyNavigation.tab: panel.nextFocus(primary, 1)
            KeyNavigation.backtab: panel.nextFocus(primary, -1)
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 28
            Layout.preferredHeight: 1
            color: "#30435e"
        }

        RowLayout {
            id: footerRow
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            Layout.maximumHeight: 66
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 66
                Layout.preferredWidth: 1

                FooterSelector {
                    id: sessionSelector
                    objectName: "sessionSelector"
                    anchors.fill: parent
                    anchors.margins: 5
                    iconText: "▱"
                    model: greeterController.sessions
                    textRole: "name"
                    valueRole: "id"
                    enabled: greeterConfigError.length === 0 && count > 0
                             && !["starting", "authenticated"].includes(greeterController.state)
                    Component.onCompleted: {
                        const wanted = indexOfValue(greeterController.selectedSession)
                        if (wanted >= 0)
                            currentIndex = wanted
                    }
                    onActivated: {
                        greeterController.selectedSession = currentValue
                        panel.focusPassword()
                    }
                    KeyNavigation.tab: panel.nextFocus(sessionSelector, 1)
                    KeyNavigation.backtab: panel.nextFocus(sessionSelector, -1)
                }
            }

            Rectangle {
                id: footerDivider
                Layout.preferredWidth: 1
                Layout.preferredHeight: 38
                color: "#30435e"
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 66
                Layout.preferredWidth: 1

                FooterSelector {
                    id: keyboardSelector
                    objectName: "keyboardSelector"
                    anchors.fill: parent
                    anchors.margins: 5
                    iconText: "⌨"
                    model: greeterCompositor.layouts
                    textRole: "label"
                    valueRole: "id"
                    enabled: greeterCompositor.canCycleLayout
                    function syncSelection() {
                        const wanted = indexOfValue(greeterCompositor.keyboardLayoutId)
                        if (wanted >= 0)
                            currentIndex = wanted
                    }
                    Component.onCompleted: syncSelection()
                    onActivated: {
                        greeterCompositor.selectLayout(currentValue)
                        syncSelection()
                        panel.focusPassword()
                    }
                    KeyNavigation.tab: panel.nextFocus(keyboardSelector, 1)
                    KeyNavigation.backtab: panel.nextFocus(keyboardSelector, -1)

                    Connections {
                        target: greeterCompositor
                        function onLayoutChanged() { keyboardSelector.syncSelection() }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#30435e"
        }

        Controls.Label {
            Layout.fillWidth: true
            Layout.topMargin: 18
            text: greeterController.status.length > 0 ? greeterController.status
                  : greeterController.state === "connecting" ? "Connecting to authentication service"
                  : greeterController.state === "waiting" ? "Waiting for authentication"
                  : greeterController.state === "starting" ? "Starting selected session"
                  : greeterController.state === "authenticated" ? "Authenticated"
                  : "Ready to authenticate"
            color: greeterController.status.length > 0 ? "#ff89a2" : "#526b8e"
            font.pointSize: 12
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }

}
