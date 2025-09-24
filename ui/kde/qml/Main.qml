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
    // Movie count propagated from the ListView so footer can access it
    property int movieCount: 0

    // Header toolbar with primary actions
    header: ToolBar {
        position: ToolBar.Header
        RowLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing
            ToolButton {
                icon.name: "list-add"
                text: qsTr("Add")
                display: AbstractButton.TextUnderIcon
                icon.width: Kirigami.Units.iconSizes.medium
                icon.height: Kirigami.Units.iconSizes.medium
                onClicked: addDialog.open()
            }
            ToolButton {
                icon.name: "edit-delete"
                text: qsTr("Delete")
                display: AbstractButton.TextUnderIcon
                icon.width: Kirigami.Units.iconSizes.medium
                icon.height: Kirigami.Units.iconSizes.medium
                onClicked: openDeleteDialog()
            }
            ToolButton {
                icon.name: "view-refresh"
                text: qsTr("Refresh")
                display: AbstractButton.TextUnderIcon
                icon.width: Kirigami.Units.iconSizes.medium
                icon.height: Kirigami.Units.iconSizes.medium
                onClicked: top100Model.reload()
            }
            ToolButton {
                icon.name: "document-save"
                text: qsTr("Update (OMDb)")
                display: AbstractButton.TextUnderIcon
                icon.width: Kirigami.Units.iconSizes.medium
                icon.height: Kirigami.Units.iconSizes.medium
                onClicked: {
                    if (list.currentIndex < 0) { window.showPassiveNotification(qsTr("Select a movie first.")); return }
                    var info = top100Model.get(list.currentIndex)
                    if (!info || !info.imdbID || info.imdbID.length === 0) { window.showPassiveNotification(qsTr("No IMDb ID for this movie.")); return }
                    window.showPassiveNotification(qsTr("Updating from OMDb..."))
                    var ok = top100Model.updateFromOmdbByImdbId(info.imdbID)
                    if (!ok) {
                        window.showPassiveNotification(qsTr("Update failed."))
                    } else {
                        var once = function() {
                            top100Model.reloadCompleted.disconnect(once)
                            window.showPassiveNotification(qsTr("Updated from OMDb."))
                        }
                        top100Model.reloadCompleted.connect(once)
                    }
                }
            }
            ToolButton {
                icon.name: "cloud-upload"
                text: qsTr("Post BlueSky")
                display: AbstractButton.TextUnderIcon
                icon.width: Kirigami.Units.iconSizes.medium
                icon.height: Kirigami.Units.iconSizes.medium
                onClicked: postSelectedToBlueSky()
            }
            ToolButton {
                icon.name: "mail-send"
                text: qsTr("Post Mastodon")
                display: AbstractButton.TextUnderIcon
                icon.width: Kirigami.Units.iconSizes.medium
                icon.height: Kirigami.Units.iconSizes.medium
                onClicked: postSelectedToMastodon()
            }
            ToolButton {
                icon.name: "favorites"
                text: qsTr("Rank")
                display: AbstractButton.TextUnderIcon
                icon.width: Kirigami.Units.iconSizes.medium
                icon.height: Kirigami.Units.iconSizes.medium
                onClicked: openRankDialog()
            }
        }
    }

    globalDrawer: Kirigami.GlobalDrawer {
        actions: [
            Kirigami.Action {
                text: UiStrings_MenuFile
                tooltip: ""
                children: [
                    Kirigami.Action {
                        text: qsTr("Refresh")
                        tooltip: ""
                        onTriggered: top100Model.reload()
                    },
                    Kirigami.Action {
                        text: UiStrings_ActionQuit
                        tooltip: ""
                        onTriggered: Qt.quit()
                    }
                ]
            },
            Kirigami.Action {
                text: qsTr("Sort")
                tooltip: ""
                children: [
                    Kirigami.Action { text: qsTr("Insertion order"); tooltip: ""; onTriggered: top100Model.setSortOrder(0) },
                    Kirigami.Action { text: qsTr("By year"); tooltip: ""; onTriggered: top100Model.setSortOrder(1) },
                    Kirigami.Action { text: qsTr("Alphabetical"); tooltip: ""; onTriggered: top100Model.setSortOrder(2) },
                    Kirigami.Action { text: qsTr("By my rank"); tooltip: ""; onTriggered: top100Model.setSortOrder(3) },
                    Kirigami.Action { text: qsTr("By my score"); tooltip: ""; onTriggered: top100Model.setSortOrder(4) }
                ]
            },
            Kirigami.Action {
                text: UiStrings_MenuHelp
                tooltip: ""
                children: [
                    Kirigami.Action {
                        text: UiStrings_ActionAbout
                        tooltip: ""
                        onTriggered: aboutDialog.open()
                    }
                ]
            }
        ]
    }

    // Footer status bar with left-aligned movie count
    footer: ToolBar {
        position: ToolBar.Footer
        RowLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing
            Label {
                text: window.movieCount + (window.movieCount === 1 ? qsTr(" movie") : qsTr(" movies"))
                Layout.alignment: Qt.AlignLeft
            }
            Item { Layout.fillWidth: true } // spacer to keep count on the left
        }
    }

    Dialog {
        id: aboutDialog
        modal: true
        width: Math.min(Kirigami.Units.gridUnit * 40, window.width * 0.6)
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
                        Component.onCompleted: {
                            if (count > 0) currentIndex = 0
                            window.movieCount = count
                        }
                        onCountChanged: {
                            if (currentIndex < 0 && count > 0) currentIndex = 0
                            window.movieCount = count
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

    // ---------------- Ranking dialog ----------------
    function openRankDialog() {
        if (!top100Model || (top100Model.count && top100Model.count() < 2)) {
            window.showPassiveNotification(qsTr("Need at least 2 movies to rank."));
            return;
        }
        rankDialog.resetAndOpen();
    }

    Dialog {
        id: rankDialog
        modal: true
    // Keep the window title empty; use the centered heading below
    title: ""
        // Suppress the default QQC2 header entirely to avoid a second title row
        header: null
    // Fixed size and not resizable
    width: Kirigami.Units.gridUnit * 70
    height: Kirigami.Units.gridUnit * 45
    resizeToItem: false
        standardButtons: Dialog.NoButton
        // Center the dialog on the main window
        anchors.centerIn: parent

        property int leftIndex: -1
        property int rightIndex: -1

        function leftMovie() {
            if (!top100Model || leftIndex < 0) return ({})
            if (typeof top100Model.get !== 'function') return ({})
            return top100Model.get(leftIndex)
        }
        function rightMovie() {
            if (!top100Model || rightIndex < 0) return ({})
            if (typeof top100Model.get !== 'function') return ({})
            return top100Model.get(rightIndex)
        }
        function joinList(list) {
            if (!list || list.length === 0) return ""
            var s = ""
            for (var i = 0; i < list.length; ++i) { if (i) s += ", "; s += list[i] }
            return s
        }

        function pickTwo() {
            var n = (top100Model && typeof top100Model.count === 'function') ? top100Model.count() : 0
            if (n < 2) { leftIndex = rightIndex = -1; return }
            var a = Math.floor(Math.random() * n)
            var b = Math.floor(Math.random() * n)
            if (b === a) b = (a + 1) % n
            leftIndex = a
            rightIndex = b
        }
        function resetAndOpen() {
            pickTwo()
            open()
        }
        function chooseLeft() {
            if (leftIndex < 0 || rightIndex < 0) return
            top100Model.recordPairwiseResult(leftIndex, rightIndex, 1)
            pickTwo()
        }
        function chooseRight() {
            if (leftIndex < 0 || rightIndex < 0) return
            top100Model.recordPairwiseResult(leftIndex, rightIndex, 0)
            pickTwo()
        }
        function passPair() {
            pickTwo()
        }

        onOpened: {
            // Center dialog within window and set key focus
            x = Math.max(0, Math.round((window.width - width) / 2))
            y = Math.max(0, Math.round((window.height - height) / 2))
            leftKeyTarget.focus = true
        }

        contentItem: Item {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing // gap around dialog content
            Keys.forwardTo: [ leftKeyTarget ]

            // Invisible item to capture keys
            Item {
                id: leftKeyTarget
                focus: true
                Keys.onLeftPressed: rankDialog.chooseLeft()
                Keys.onRightPressed: rankDialog.chooseRight()
                Keys.onDownPressed: rankDialog.passPair()
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: Kirigami.Units.largeSpacing
                // Centered bold heading for dialog
                Kirigami.Heading {
                    level: 2
                    text: qsTr("Rank movies")
                    font.bold: true
                    font.weight: Font.Bold
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }

                // Prompt
                Label {
                    text: qsTr("Which movie do you prefer?")
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }

                // Two-pane row
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Kirigami.Units.largeSpacing

                    // Left pane
                    Frame {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        padding: Kirigami.Units.smallSpacing
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: Kirigami.Units.smallSpacing
                            Kirigami.Heading {
                                level: 3; font.bold: true; font.weight: Font.Bold
                                text: (rankDialog.leftMovie().title || "") + (rankDialog.leftMovie().year ? (" (" + rankDialog.leftMovie().year + ")") : "")
                            }
                            GridLayout {
                                columns: 2; rowSpacing: 4; columnSpacing: 12
                                Label { text: UiStrings_FieldDirector; Layout.minimumWidth: Ui_LabelMinWidth }
                                Label { text: (rankDialog.leftMovie().director || "") ; wrapMode: Text.WordWrap }
                                Label { text: UiStrings_FieldActors; Layout.minimumWidth: Ui_LabelMinWidth }
                                Label { text: (rankDialog.leftMovie().actors && rankDialog.leftMovie().actors.length > 0) ? rankDialog.leftMovie().actors.join(", ") : ""; elide: Text.ElideRight }
                                Label { text: UiStrings_FieldGenres; Layout.minimumWidth: Ui_LabelMinWidth }
                                Label { text: (rankDialog.leftMovie().genres && rankDialog.leftMovie().genres.length > 0) ? rankDialog.leftMovie().genres.join(", ") : ""; wrapMode: Text.WordWrap }
                                Label { text: UiStrings_FieldRuntime; Layout.minimumWidth: Ui_LabelMinWidth }
                                Label { text: (rankDialog.leftMovie().runtimeMinutes ? (rankDialog.leftMovie().runtimeMinutes + " min") : "") }
                            }
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Image {
                                    anchors.centerIn: parent
                                    width: parent.width * Ui_PosterMaxWidthRatio
                                    height: parent.height * Ui_PosterMaxHeightRatio
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                    source: (rankDialog.leftMovie().posterUrl || "")
                                    visible: source !== "" && status === Image.Ready
                                }
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: rankDialog.chooseLeft()
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true
                            ToolTip.visible: containsMouse
                            ToolTip.delay: 300
                            ToolTip.text: qsTr("Click to choose this movie")
                        }
                    }

                    // Right pane
                    Frame {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        padding: Kirigami.Units.smallSpacing
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: Kirigami.Units.smallSpacing
                            Kirigami.Heading {
                                level: 3; font.bold: true; font.weight: Font.Bold
                                text: (rankDialog.rightMovie().title || "") + (rankDialog.rightMovie().year ? (" (" + rankDialog.rightMovie().year + ")") : "")
                            }
                            GridLayout {
                                columns: 2; rowSpacing: 4; columnSpacing: 12
                                Label { text: UiStrings_FieldDirector; Layout.minimumWidth: Ui_LabelMinWidth }
                                Label { text: (rankDialog.rightMovie().director || "") ; wrapMode: Text.WordWrap }
                                Label { text: UiStrings_FieldActors; Layout.minimumWidth: Ui_LabelMinWidth }
                                Label { text: (rankDialog.rightMovie().actors && rankDialog.rightMovie().actors.length > 0) ? rankDialog.rightMovie().actors.join(", ") : ""; elide: Text.ElideRight }
                                Label { text: UiStrings_FieldGenres; Layout.minimumWidth: Ui_LabelMinWidth }
                                Label { text: (rankDialog.rightMovie().genres && rankDialog.rightMovie().genres.length > 0) ? rankDialog.rightMovie().genres.join(", ") : ""; wrapMode: Text.WordWrap }
                                Label { text: UiStrings_FieldRuntime; Layout.minimumWidth: Ui_LabelMinWidth }
                                Label { text: (rankDialog.rightMovie().runtimeMinutes ? (rankDialog.rightMovie().runtimeMinutes + " min") : "") }
                            }
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Image {
                                    anchors.centerIn: parent
                                    width: parent.width * Ui_PosterMaxWidthRatio
                                    height: parent.height * Ui_PosterMaxHeightRatio
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                    source: (rankDialog.rightMovie().posterUrl || "")
                                    visible: source !== "" && status === Image.Ready
                                }
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: rankDialog.chooseRight()
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true
                            ToolTip.visible: containsMouse
                            ToolTip.delay: 300
                            ToolTip.text: qsTr("Click to choose this movie")
                        }
                    }
                }

                // Bottom action row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    Item { Layout.fillWidth: true }
                    Button {
                        text: qsTr("Pass")
                        highlighted: true // default-style emphasis
                        onClicked: rankDialog.passPair()
                    }
                    Button {
                        text: qsTr("Finish Ranking")
                        onClicked: rankDialog.close()
                    }
                }
            }
            // Treat Enter/Return as Pass for quick flow
            Keys.onEnterPressed: rankDialog.passPair()
            Keys.onReturnPressed: rankDialog.passPair()
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
        function onRequestSelectRow(row) {
            list.currentIndex = row
        }
        function onReloadCompleted() {
            // Nudge selection to refresh bound details when index stays the same
            if (list.currentIndex >= 0) {
                var idx = list.currentIndex
                list.currentIndex = -1
                list.currentIndex = idx
            }
        }
    }
    
    // (removed) legacy resultsDialog; integrated selection occurs in addDialog

    QtObject {
        id: selectionModel
        property var results: []
    }

    Dialog {
        id: addDialog
        modal: true
        title: qsTr("Add Movie (OMDb)")
        width: window.width * 0.5
        height: window.height * 0.6
        x: (window.width - width) / 2
        y: (window.height - height) / 2
        standardButtons: Dialog.NoButton
        property var selected: null
        onVisibleChanged: if (visible) { x = (window.width - width)/2; y = (window.height - height)/2 }
        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing
            // Search row
            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                Label { text: qsTr("Search for movie"); Layout.alignment: Qt.AlignVCenter }
                TextField {
                    id: queryField
                    Layout.fillWidth: true
                    placeholderText: qsTr("title keyword")
                    onAccepted: searchBtn.clicked()
                }
                Button {
                    id: searchBtn
                    text: qsTr("Search")
                    highlighted: true
                    focus: true
                    onClicked: {
                        var q = queryField.text
                        if (!q || q.trim().length === 0) return
                        var results = top100Model.searchOmdb(q)
                        if (!results || results.length === 0) { window.showPassiveNotification(qsTr("No results.")); return }
                        selectionModel.results = results
                        resultsList.currentIndex = 0
                    }
                }
            }

            // Content split: results list on left, preview on right
            SplitView {
                id: addSplit
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: Qt.Horizontal
                // Results
                ListView {
                    id: resultsList
                    SplitView.preferredWidth: parent.width * 0.4
                    model: selectionModel.results
                    delegate: ItemDelegate {
                        width: parent.width
                        text: (modelData.title + " (" + modelData.year + ") [" + modelData.imdbID + "]")
                        onClicked: resultsList.currentIndex = index
                    }
                    onCurrentIndexChanged: {
                        if (currentIndex >= 0 && model && model.length > currentIndex) {
                            var imdb = model[currentIndex].imdbID
                            addDialog.selected = top100Model.omdbGetByIdMap(imdb)
                        } else {
                            addDialog.selected = null
                        }
                    }
                }
                // Preview panel
                Flickable {
                    SplitView.preferredWidth: parent.width * 0.6
                    contentWidth: width
                    contentHeight: previewCol.implicitHeight
                    clip: true
                    ColumnLayout {
                        id: previewCol
                        width: parent.width
                        spacing: Kirigami.Units.smallSpacing
                        Kirigami.Heading {
                            level: 3
                            text: addDialog.selected ? ((addDialog.selected.title || "") + (addDialog.selected.year ? (" (" + addDialog.selected.year + ")") : "")) : ""
                            visible: !!addDialog.selected
                        }
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 220
                            Image {
                                anchors.centerIn: parent
                                width: parent.width * Ui_PosterMaxWidthRatio
                                height: parent.height * Ui_PosterMaxHeightRatio
                                fillMode: Image.PreserveAspectFit
                                source: (addDialog.selected && addDialog.selected.posterUrl) ? addDialog.selected.posterUrl : ""
                                visible: source !== "" && status === Image.Ready
                            }
                        }
                        Text {
                            width: parent.width
                            wrapMode: Text.WordWrap
                            text: addDialog.selected ? ((addDialog.selected.plotShort && addDialog.selected.plotShort.length > 0) ? addDialog.selected.plotShort : (addDialog.selected.plotFull || "")) : ""
                            visible: !!addDialog.selected
                        }
                    }
                }
            }

            // Buttons row
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: Kirigami.Units.smallSpacing
                Button {
                    text: qsTr("Enter manually")
                    onClicked: {
                        var dialog = Qt.createQmlObject('
                            import QtQuick 2.15; import QtQuick.Controls 2.15; import org.kde.kirigami 2.20 as Kirigami; Dialog { id: d; modal: true; title: "Add Movie by IMDb ID"; standardButtons: Dialog.Ok | Dialog.Cancel; property string imdb: ""; contentItem: Column { spacing: 8; Label { text: "Enter IMDb ID (e.g., tt0133093):" } TextField { id: tf; placeholderText: "tt........"; onAccepted: d.accept(); onTextChanged: d.imdb = text } } }
                        ', window, "ManualAddDialog")
                        dialog.accepted.connect(function() {
                            if (dialog.imdb && dialog.imdb.length > 0) {
                                if (top100Model.addMovieByImdbId(dialog.imdb)) {
                                    window.showPassiveNotification(qsTr("Movie added."))
                                } else {
                                    window.showPassiveNotification(qsTr("Add failed."))
                                }
                            }
                        })
                        dialog.open()
                        addDialog.close()
                    }
                }
                Button { text: qsTr("Cancel"); onClicked: addDialog.close() }
                Button {
                    text: qsTr("Add")
                    enabled: addDialog.selected && addDialog.selected.title && addDialog.selected.year
                    onClicked: {
                        if (resultsList.currentIndex >= 0) {
                            var imdbSel = selectionModel.results[resultsList.currentIndex].imdbID
                            if (top100Model.addMovieByImdbId(imdbSel)) {
                                window.showPassiveNotification(qsTr("Movie added."))
                                addDialog.close()
                            } else {
                                window.showPassiveNotification(qsTr("Add failed."))
                            }
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: deleteDialog
        modal: true
        width: Math.min(420, window.width * 0.6)
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
