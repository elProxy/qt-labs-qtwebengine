import QtQuick 2.1
import QtQuick.Controls 1.0


Menu {
    id: contextMenu
    on__MenuClosed: contextModel.done()
    property QtObject contextModel: model

    MenuContents {
            model: contextModel
            menu:contextMenu
        }
    Component.onCompleted: contextMenu.popup();
}
