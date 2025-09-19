// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/kde/qml/Main.qml
// Purpose: Main QML for KDE app (two-pane layout, poster, plot, actions).
// Language: QML (Qt Quick/Kirigami)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami

Kirigami.ApplicationWindow {
    id: window
    title: qsTr("Top100 — KDE UI")
    width: 480
    height: 320
    visible: true

    // Simple header toolbar with primary actions (Qt Quick Controls)
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing
            ToolButton { text: qsTr("Add"); onClicked: window.showPassiveNotification(qsTr("Add is not implemented yet.")) }
            ToolButton { text: qsTr("Delete"); onClicked: window.showPassiveNotification(qsTr("Delete is not implemented yet.")) }
            ToolButton { text: qsTr("Refresh"); onClicked: top100Model.reload() }
            ToolButton { text: qsTr("Post BlueSky"); onClicked: postSelectedToBlueSky() }
            ToolButton { text: qsTr("Post Mastodon"); onClicked: postSelectedToMastodon() }
        }
    }

    globalDrawer: Kirigami.GlobalDrawer {
        actions: [
            Kirigami.Action {
                text: UiStrings_MenuFile
                children: [
                    Kirigami.Action {
                        text: qsTr("Refresh")
                        onTriggered: top100Model.reload()
                    },
                    Kirigami.Action {
                        text: UiStrings_ActionQuit
                        onTriggered: Qt.quit()
                    }
                ]
            },
            Kirigami.Action {
                text: UiStrings_MenuHelp
                children: [
                    Kirigami.Action {
                        text: UiStrings_ActionAbout
                        onTriggered: aboutDialog.open()
                    }
                ]
            }
        ]
    }

    Dialog {
        id: aboutDialog
        modal: true
        title: UiStrings_ActionAbout
        standardButtons: Dialog.Ok
        contentItem: Column {
            spacing: Kirigami.Units.smallSpacing
            Kirigami.Heading { level: 3; text: UiStrings_AboutText }
        }
    }

    pageStack.initialPage: Kirigami.ScrollablePage {
        title: qsTr("Movies")

        SplitView {
            id: lv
            anchors.fill: parent

            ListView {
                id: list
                SplitView.preferredWidth: Math.round(parent.width * 0.45)
                SplitView.minimumWidth: 200
                model: top100Model
                Component.onCompleted: {
                    if (count > 0) currentIndex = 0
                }
                onCountChanged: {
                    if (currentIndex < 0 && count > 0) currentIndex = 0
                }
                delegate: Item {
                    width: ListView.view.width
                    height: label.implicitHeight + Kirigami.Units.smallSpacing * 2
                    property int rowIndex: index
                    Kirigami.Heading {
                        id: label
                        level: 4
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: Kirigami.Units.smallSpacing
                        text: (rank > 0 ? ("#" + rank + " ") : "") + title + " (" + year + ")"
                        elide: Text.ElideRight
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: list.currentIndex = rowIndex
                    }
                }
                highlight: Rectangle { color: Kirigami.Theme.highlightColor; opacity: 0.2 }
                focus: true
                keyNavigationWraps: true
                interactive: true
                clip: true

            }

            // Details pane
            Flickable {
                id: detailsPane
                SplitView.preferredWidth: Math.round(parent.width * 0.55)
                contentWidth: width
                contentHeight: detailsColumn.implicitHeight
                clip: true
                Column {
                    id: detailsColumn
                    anchors.fill: parent
                    anchors.margins: Kirigami.Units.largeSpacing
                    width: parent.width - anchors.margins * 2
                    spacing: Kirigami.Units.largeSpacing
                    property var selected: (top100Model ? top100Model.get(list.currentIndex) : null)

                    Kirigami.Heading {
                        level: 2
                        text: detailsColumn.selected && detailsColumn.selected.title ? (detailsColumn.selected.title + " (" + detailsColumn.selected.year + ")") : ""
                        wrapMode: Text.Wrap
                        width: detailsPane.width - (Kirigami.Units.largeSpacing * 2)
                    }

                    Image {
                        id: poster
                        source: detailsColumn.selected && detailsColumn.selected.posterUrl ? detailsColumn.selected.posterUrl : ""
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: true
                        width: Math.min(400, detailsPane.width * 0.6)
                        height: width * 1.5
                        visible: source !== "" && status === Image.Ready
                    }

                    Kirigami.Heading { level: 4; text: qsTr("Plot"); visible: detailsColumn.selected && detailsColumn.selected.plotFull && detailsColumn.selected.plotFull.length > 0 }
                    Text {
                        text: detailsColumn.selected && detailsColumn.selected.plotFull ? detailsColumn.selected.plotFull : ""
                        wrapMode: Text.WordWrap
                        width: detailsPane.width - (Kirigami.Units.largeSpacing * 2)
                    }
                }
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width * 0.8
            visible: list.count === 0
            text: qsTr("No movies yet. Use the CLI to add some, then reopen the app.")
        }
    }

    // Helpers to post the currently selected item
    function postSelectedToBlueSky() {
        var idx = list.currentIndex
        if (idx < 0) {
            window.showPassiveNotification(qsTr("Please select a movie first."))
            return
        }
        window.showPassiveNotification(qsTr("Posting to BlueSky..."))
        top100Model.postToBlueSkyAsync(idx)
    }

    function postSelectedToMastodon() {
        var idx = list.currentIndex
        if (idx < 0) {
            window.showPassiveNotification(qsTr("Please select a movie first."))
            return
        }
        window.showPassiveNotification(qsTr("Posting to Mastodon..."))
        top100Model.postToMastodonAsync(idx)
    }

    Connections {
        target: top100Model
        function onPostingFinished(service, row, success) {
            var msg = (success ? qsTr("Posted to ") : qsTr("Posting failed: ")) + service
            window.showPassiveNotification(msg)
        }
    }
}
