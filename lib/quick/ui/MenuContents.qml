import QtQuick 2.1
import QtQuick.Controls 1.0

Instantiator {
    property var menu
//    property var modelData
    delegate: Instantiator {
        id: instantiatorDelegate
        property string text: modelData.text
        property var subMenuModel: model.subMenu

        delegate: model.isSeparator ? separator : model.subMenu ? subMenu : item

        property Component item: MenuItem {
            text: text
//            enabled: model.enabled
        }

        property Component separator: MenuSeparator { }

        property Component subMenu: Menu {
            title: text
        }
    }

    onObjectAdded: {
        menu.insertItem(index, object.object)
        var submenu = object.object
        if (submenu.type === MenuItemType.Menu) {
            var instantiatorComponent = Qt.createComponent("MenuContents.qml", submenu)
            var modelInstantiator = instantiatorComponent.createObject(submenu, { "model": object.subMenuModel, "menu": submenu })
            submenu.addItem(0, modelInstantiator)
        }
    }
}
