// qmllint disable unqualified
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Holonight.Controls

ApplicationWindow {
    id: root
    objectName: "greeterWindow"
    width: greeterDemo ? 1280 : Screen.width
    height: greeterDemo ? 720 : Screen.height
    visible: true
    visibility: greeterDemo ? Window.Windowed : Window.FullScreen
    title: "HoloNight Greeter"
    color: "#080a10"
    readonly property bool compact: width < 900

    Image {
        anchors.fill: parent
        source: greeterDemo ? "qrc:/qt/qml/Holonight/Greeter/assets/wallpaper1.png"
                            : "file:" + greeterBackground
        fillMode: Image.PreserveAspectCrop
    }
    Rectangle { anchors.fill: parent; color: "#78020710" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.compact ? 32 : 72
        spacing: 32
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: 64
            ColumnLayout {
                visible: !root.compact
                Layout.fillWidth: true
                Label { text: Qt.formatTime(new Date(), "hh:mm"); font.pixelSize: 88; color: "white" }
                Label { text: Qt.formatDate(new Date(), "dddd, MMMM d"); font.pixelSize: 24; color: "#d7dbe8" }
                Label { text: "HOLO//NIGHT"; font.letterSpacing: 4; color: "#80e6ff" }
                Item { Layout.fillHeight: true }
            }
            LoginPanel {
                Layout.preferredWidth: root.compact ? Math.min(480, root.width - 64) : Math.min(480, Math.max(440, root.width * 0.32))
                Layout.preferredHeight: Math.min(620, root.height - 100)
                Layout.alignment: Qt.AlignCenter
            }
        }
        Label { visible: greeterConfigWarning.length > 0; text: greeterConfigWarning; color: "#ffd479"; wrapMode: Text.Wrap }
    }
}
