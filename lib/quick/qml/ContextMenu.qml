import QtQuick 2.1
import QtQuick.Controls 1.0


Menu {
    // Avoid conflicting with ListView's model property
    property QtObject contextModel: model

    id: contextMenu
    // Private API for now
    on__MenuClosed: contextModel.done()

    Instantiator {
        model: contextModel
        delegate: model.isSeparator? separator : item
        property Component item: Component {
            MenuItem {
                text: model.text
                enabled: model.enabled
                onTriggered: contextModel.accept(index)
            }
        }
        property Component separator: Component {
            id: separator
            MenuSeparator { }
        }
        onObjectAdded: contextMenu.insertItem(index, object)
        onObjectRemoved: contextMenu.removeItem(object)
    }
    Component.onCompleted: contextMenu.popup()
}
