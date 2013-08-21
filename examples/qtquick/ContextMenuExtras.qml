import QtQuick 2.1
import QtWebEngine.UIDelegates 1.0

VisualItemModel {
    WebMenuItem {
        text: "An application specific entry"
        onTriggered: console.log("Application specific action triggered")
    }
    WebMenu {
        title: "Extras Submenu"
        WebMenuItem {
            text: "something"
            onTriggered: console.log("something triggered")
        }
        WebMenuItem {
            text: "something else"
            enabled: false
        }
    }
}

