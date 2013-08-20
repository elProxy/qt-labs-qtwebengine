import QtQuick 2.1
import QtQuick.Controls 1.0


ListModel {

    ListElement {
        text: "prepended stuff"
        enabled: true
    }

    Component.onCompleted: { insert(0, {"text": "Foobar", "enabled": true, "onTriggered": "console.log('prepended foobar triggered')" }); }
}
