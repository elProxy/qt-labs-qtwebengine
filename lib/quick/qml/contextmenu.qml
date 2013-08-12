import QtQuick 2.0

MouseArea{
    // Avoid conflicting with ListView's model property
    property QtObject contextModel: model

    property var fittingWidth: 0;
    property var horizontalMargin: 20;

    // filter out mouse events outside the menu
    anchors.fill: parent
    z: 2
    hoverEnabled: true
    onClicked: model.reject()

    Rectangle {
        id: contextMenu
        color: "#C8C6C4"
        width: fittingWidth
        height: Math.min(model.count * 30, (parent.y + parent.height - model.pos.y))
        x: model.pos.x
        y: model.pos.y
        radius: 5
        opacity: 0.9
        ListView {
            clip: true
            anchors.fill: parent
            model: contextModel
            interactive: (contextMenu.height != model.count * 30)
            delegate: Rectangle {
                width: fittingWidth
                height: 30
                radius: 5
                color: menuItem.containsMouse? "steelblue" : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: model.text
                    color: model.enabled ? (menuItem.containsMouse? "white" :  "black") : "#4B4B4B"
                    onWidthChanged: { if (width + horizontalMargin > fittingWidth) fittingWidth = width + horizontalMargin; }
                }
                MouseArea {
                    id: menuItem
                    anchors.fill: parent
                    enabled: model.enabled
                    hoverEnabled: true
                    onClicked: contextModel.accept(model.index)
                }
            }
        }

    }
}
