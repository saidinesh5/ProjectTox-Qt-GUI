import QtQuick 2.2
import QtQuick.Window 2.1

import org.garageresearch.toxin 0.1

Window {
    id: toxin

    visible: true
    width: 480
    height: 800

    Rectangle {
        anchors.fill: parent
        color: 'grey'

        Text {
            anchors.centerIn: parent
            text: engine.username
        }
    }

    Engine {
        id: engine

    }

}
