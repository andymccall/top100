// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
/*!
 * \file ui/kde/qml/Main.qml
 * \brief Main Kirigami QML page (two-pane UI, add/delete, posting).
 */
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
import QtQuick.Window 2.15
import org.kde.kirigami 2.20 as Kirigami

Kirigami.ApplicationWindow {
    id: window
    title: qsTr("Top100 — KDE UI")
    width: Math.min(Screen.width, Math.round(Ui_InitWidth * 1.2))
    height: Math.min(Screen.height, Math.round(Ui_InitHeight * 1.2))
    visible: true

    // Simple header toolbar with primary actions (Qt Quick Controls)
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing
            ToolButton { icon.name: "list-add"; text: qsTr("Add"); onClicked: addDialog.open() }
            ToolButton { icon.name: "edit-delete"; text: qsTr("Delete"); onClicked: openDeleteDialog() }
            ToolButton { icon.name: "view-refresh"; text: qsTr("Refresh"); onClicked: top100Model.reload() }
            ToolButton { icon.name: "cloud-upload"; text: qsTr("Post BlueSky"); onClicked: postSelectedToBlueSky() }
            ToolButton { icon.name: "mail-send"; text: qsTr("Post Mastodon"); onClicked: postSelectedToMastodon() }
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
                text: qsTr("Sort")
                children: [
                    Kirigami.Action { text: qsTr("Insertion order"); onTriggered: top100Model.setSortOrder(0) },
                    Kirigami.Action { text: qsTr("By year"); onTriggered: top100Model.setSortOrder(1) },
                    Kirigami.Action { text: qsTr("Alphabetical"); onTriggered: top100Model.setSortOrder(2) },
                    Kirigami.Action { text: qsTr("By my rank"); onTriggered: top100Model.setSortOrder(3) },
                    Kirigami.Action { text: qsTr("By my score"); onTriggered: top100Model.setSortOrder(4) }
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
        id: page
        // Safe selection helpers
        property bool hasSelection: list && list.currentIndex >= 0 && list.count > 0
        property var currentMovie: (list && top100Model && list.currentIndex >= 0) ? top100Model.get(list.currentIndex) : ({})
        title: qsTr("Top100")

        SplitView {
            id: mainSplit
            anchors.fill: parent

            // Left frame: heading, sort order row, and movie list
            Frame {
                id: leftFrame
                SplitView.preferredWidth: Math.round(parent.width * 0.45)
                SplitView.minimumWidth: 200
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6
                    Kirigami.Heading { level: 2; text: UiStrings_HeadingMovies; font.bold: true; font.weight: Font.Bold }
                    RowLayout {
                        spacing: 6
                        Label { text: UiStrings_LabelSortOrder }
                        ComboBox {
                            id: sortCombo
                            Layout.fillWidth: true
                            model: [UiStrings_SortInsertion, UiStrings_SortByYear, UiStrings_SortAlpha, UiStrings_SortByRank, UiStrings_SortByScore]
                            Component.onCompleted: currentIndex = top100Model.sortOrder
                            onCurrentIndexChanged: {
                                var oldIdx = list.currentIndex
                                var imdb = ""
                                if (oldIdx >= 0 && top100Model) {
                                    var info = top100Model.get(oldIdx)
                                    imdb = info ? (info.imdbID || "") : ""
                                }
                                var once = function() {
                                    top100Model.reloadCompleted.disconnect(once)
                                    if (imdb.length > 0) {
                                        for (var i = 0; i < list.count; ++i) {
                                            var m = top100Model.get(i)
                                            if (m && m.imdbID === imdb) { list.currentIndex = i; break }
                                        }
                                    }
                                }
                                top100Model.reloadCompleted.connect(once)
                                top100Model.setSortOrder(currentIndex)
                            }
                        }
                    }
                    ListView {
                        id: list
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: top100Model
                        Component.onCompleted: { if (count > 0) currentIndex = 0 }
                        onCountChanged: { if (currentIndex < 0 && count > 0) currentIndex = 0 }
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
                            MouseArea { anchors.fill: parent; onClicked: list.currentIndex = rowIndex }
                        }
                        highlight: Rectangle { color: Kirigami.Theme.highlightColor; opacity: 0.2 }
                        focus: true
                        interactive: true
                        clip: true
                    }
                }
            }

            // Right frame: details with Poster (75%) and Plot groups
            Frame {
                id: rightFrame
                SplitView.preferredWidth: Math.round(parent.width * 0.55)
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6
                    Kirigami.Heading { level: 2; text: UiStrings_HeadingDetails; font.bold: true; font.weight: Font.Bold }
                    Kirigami.Heading {
                        level: 3; font.bold: true; font.weight: Font.Bold
                        text: (page.currentMovie.title || "") + (page.currentMovie.year ? (" (" + page.currentMovie.year + ")") : "")
                        visible: page.hasSelection
                    }
                    // Details form under title
                    GridLayout {
                        id: detailsGrid
                        columns: 2
                        rowSpacing: 4
                        columnSpacing: 12
                        visible: page.hasSelection
                        Label { text: UiStrings_FieldDirector; Layout.minimumWidth: Ui_LabelMinWidth }
                        Label { text: (page.currentMovie.director || "") ; wrapMode: Text.WordWrap }
                        Label { text: UiStrings_FieldActors; Layout.minimumWidth: Ui_LabelMinWidth }
                        Label { text: (page.currentMovie.actors ? page.currentMovie.actors.join(", ") : ""); wrapMode: Text.NoWrap; elide: Text.ElideRight }
                        Label { text: UiStrings_FieldGenres; Layout.minimumWidth: Ui_LabelMinWidth }
                        Label { text: (page.currentMovie.genres ? page.currentMovie.genres.join(", ") : "") ; wrapMode: Text.WordWrap }
                        Label { text: UiStrings_FieldRuntime; Layout.minimumWidth: Ui_LabelMinWidth }
                        Label { text: (page.currentMovie.runtimeMinutes ? (page.currentMovie.runtimeMinutes + " min") : "") }
                        Label { text: UiStrings_FieldImdbPage; Layout.minimumWidth: Ui_LabelMinWidth }
                        Kirigami.UrlButton { text: "Open"; url: (page.currentMovie.imdbID ? ("https://www.imdb.com/title/" + page.currentMovie.imdbID + "/") : "") }
                    }
                    Item {
                        // Poster area ~60%
                        Layout.fillWidth: true
                        Layout.preferredHeight: parent.height * 0.60
                        Image {
                            anchors.centerIn: parent
                            width: parent.width * Ui_PosterMaxWidthRatio
                            height: parent.height * Ui_PosterMaxHeightRatio
                            fillMode: Image.PreserveAspectFit
                            source: (page.hasSelection && page.currentMovie.posterUrl) ? page.currentMovie.posterUrl : ""
                            visible: source !== "" && status === Image.Ready
                        }
                    }
                    ColumnLayout {
                        // Plot area ~20%
                        Layout.fillWidth: true
                        Layout.preferredHeight: parent.height * 0.20
                        Kirigami.Heading { level: 4; text: UiStrings_GroupPlot; horizontalAlignment: Text.AlignLeft; font.bold: true; font.weight: Font.Bold }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 3
                            color: "transparent"
                            border.color: Kirigami.Theme.disabledTextColor
                            border.width: 1
                            Layout.margins: Kirigami.Units.smallSpacing
                            Flickable {
                                id: plotFlick
                                anchors.fill: parent
                                anchors.margins: Kirigami.Units.smallSpacing
                                clip: true
                                contentWidth: width
                                contentHeight: plotText.implicitHeight
                                ScrollBar.vertical: ScrollBar { id: vbar }
                                Text {
                                    id: plotText
                                    // Keep clear of scrollbar and border margins
                                    x: Kirigami.Units.smallSpacing
                                    y: Kirigami.Units.smallSpacing
                                    width: plotFlick.width - (vbar.visible ? vbar.implicitWidth : 0) - Kirigami.Units.smallSpacing*2
                                    wrapMode: Text.WordWrap
                                    text: (page.hasSelection && page.currentMovie.plotFull) ? page.currentMovie.plotFull : ""
                                }
                            }
                        }
                    }
                }
            }
        }

        // sort change helper in ComboBox inline now; function removed

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width * 0.8
            visible: list.count === 0
            text: qsTr("No movies yet. Use the CLI to add some, then reopen the app.")
        }
    }

    // Helpers to post the currently selected item
    function openDeleteDialog() {
        var idx = list.currentIndex
        if (idx < 0) { window.showPassiveNotification(qsTr("Select a movie first.")); return }
        var info = top100Model.get(idx)
        deleteDialog.imdbID = info.imdbID
        deleteDialog.titleText = (info.title + " (" + info.year + ")")
        deleteDialog.open()
    }

    function deleteSelected() {
        var idx = list.currentIndex
        if (idx < 0) { window.showPassiveNotification(qsTr("Select a movie first.")); return }
        var info = top100Model.get(idx)
        var imdb = info.imdbID
        if (!imdb || imdb.length === 0) { window.showPassiveNotification(qsTr("IMDb ID missing; cannot delete.")); return }
        // Simple confirm
        if (!confirm(qsTr("Delete this movie?"))) return
        if (top100Model.deleteByImdbId(imdb)) {
            window.showPassiveNotification(qsTr("Deleted."))
        } else {
            window.showPassiveNotification(qsTr("Delete failed."))
        }
    }
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
    
    Dialog {
        id: resultsDialog
        modal: true
        title: qsTr("Select Movie")
        standardButtons: Dialog.Cancel
        property var selectedImdb: ""
        contentItem: ListView {
            id: resultsList
            width: 400; height: 300
            model: selectionModel.results
            delegate: ItemDelegate {
                width: parent.width
                text: (modelData.title + " (" + modelData.year + ") [" + modelData.imdbID + "]")
                onClicked: {
                    resultsDialog.selectedImdb = modelData.imdbID
                    resultsDialog.accept()
                }
            }
        }
        onAccepted: {
            if (selectedImdb && selectedImdb.length > 0) {
                if (top100Model.addMovieByImdbId(selectedImdb)) {
                    window.showPassiveNotification(qsTr("Movie added."))
                } else {
                    window.showPassiveNotification(qsTr("Add failed."))
                }
            }
            selectedImdb = ""
        }
    }

    QtObject {
        id: selectionModel
        property var results: []
    }

    Dialog {
        id: addDialog
        modal: true
        title: qsTr("Add Movie (OMDb)")
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: Column {
            spacing: Kirigami.Units.smallSpacing
            TextField { id: queryField; placeholderText: qsTr("Title keyword") }
            Button { text: qsTr("Enter manually (not wired)"); onClicked: { window.showPassiveNotification(qsTr("Manual entry is not wired yet.")); addDialog.close() } }
        }
        onAccepted: {
            var q = queryField.text
            if (!q || q.trim().length === 0) return
            var results = top100Model.searchOmdb(q)
            if (!results || results.length === 0) { window.showPassiveNotification(qsTr("No results.")); return }
            selectionModel.results = results
            resultsDialog.open()
        }
    }

    Dialog {
        id: deleteDialog
        modal: true
        property string imdbID: ""
        property string titleText: ""
        title: qsTr("Delete Movie")
        standardButtons: Dialog.Yes | Dialog.No
        contentItem: Column {
            spacing: Kirigami.Units.smallSpacing
            Kirigami.Heading { level: 4; text: qsTr("Delete this movie?") }
            Text { text: deleteDialog.titleText; wrapMode: Text.WordWrap; width: 360 }
        }
        onAccepted: {
            if (deleteDialog.imdbID && deleteDialog.imdbID.length > 0) {
                if (top100Model.deleteByImdbId(deleteDialog.imdbID)) {
                    window.showPassiveNotification(qsTr("Deleted."))
                } else {
                    window.showPassiveNotification(qsTr("Delete failed."))
                }
            }
            deleteDialog.imdbID = ""
            deleteDialog.titleText = ""
        }
    }
}
