import QtQuick
Rectangle {
    id: root
    required property bool demo
    required property string backgroundPath
    readonly property bool backgroundLoaded: wallpaper.status === Image.Ready
    readonly property bool backgroundLoadFailed: wallpaper.status === Image.Error
    signal backgroundReady
    signal backgroundFailed
    color: "#050b18"
    Image {
        id: wallpaper
        anchors.fill: parent
        source: root.demo
                ? "qrc:/qt/qml/Holonight/Greeter/assets/backgrounds/wallpaper.png"
                : "file:" + root.backgroundPath
        fillMode: Image.PreserveAspectCrop
        onStatusChanged: {
            if (status === Image.Ready)
                root.backgroundReady()
            else if (status === Image.Error)
                root.backgroundFailed()
        }
    }
    Rectangle { anchors.fill: parent; color: "#1a020914" }
}
