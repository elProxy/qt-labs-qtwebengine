import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.1

MessageDialog {
    icon: StandardIcon.Question

    signal onInput();
    // FIXME: promp missing in Qt Quick Dialogs atm.

    standardButtons: StandardButton.Ok | StandardButton.Cancel
}
