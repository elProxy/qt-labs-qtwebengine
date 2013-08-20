import QtQuick 2.1
import QtQuick.Controls 1.0 as Controls

Controls.Menu {
    Controls.MenuItem {
        text: "always here and useless"
        enabled: false
    }

    Component.onCompleted: {
        console.log("Popping menu at cursor pos")
        popup()
    }
}
