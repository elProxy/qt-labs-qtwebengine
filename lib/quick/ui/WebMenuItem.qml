import QtQuick 2.1
import QtQuick.Controls 1.0

MenuItem {
    enabled: item.enabled
    text: item.text
    iconName: item.iconName
    onTriggered: item.triggered()
}

