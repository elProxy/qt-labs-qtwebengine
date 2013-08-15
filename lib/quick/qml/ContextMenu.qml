import QtQuick 2.1
import QtQuick.Controls 1.0

Loader {
    sourceComponent: Component {
        id: foo
        Menu {
            // Avoid conflicting with ListView's model property
            property QtObject contextModel: model

            id: contextMenu
            // Private API for now
            on__MenuClosed: contextModel.done()

            Instantiator {
                id: instantiator
                model: contextModel
                delegate: model.isMoche?  menu : (model.isSeparator? separator : item)
                property Component item: MenuItem {
                    text: model.text
                    enabled: model.enabled
                    onTriggered: contextModel.accept(index)
                }

                property Component separator: MenuSeparator { }

                property Component menu: Menu {
                    title: "Moche"
                    MenuItem { text: "crap" }
                    MenuSeparator{}
                }

//                    Loader {
//                    sourceComponent: foo
//                    property var title: model.text
//                    property QtObject contextModel: model.subMenu
//                }

                onObjectAdded:  {
                    contextMenu.insertItem(index, object)
                }
                onObjectRemoved: contextMenu.removeItem(object)
            }
        }
    }
    onLoaded: { console.log(item); item.popup(); }
    onStatusChanged: console.log(status)
}
