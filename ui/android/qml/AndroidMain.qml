// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
// AndroidMain.qml - Mobile-friendly UI inspired by KDE/Kirigami layout.
//-------------------------------------------------------------------------------
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: window
    visible: true
    title: qsTr("Top100")
    width: Math.min(Screen.width, Ui_InitWidth)
    height: Math.min(Screen.height, Ui_InitHeight)

    // Drawer for navigation + actions
    Drawer {
        id: navDrawer
        width: parent.width * 0.75
        modal: true
        edge: Qt.LeftEdge
        contentItem: ListView {
            model: [qsTr("Add"), qsTr("Refresh"), qsTr("Sort"), qsTr("Rank"), qsTr("About"), qsTr("Quit")]
            delegate: ItemDelegate {
                width: parent.width; text: modelData
                onClicked: {
                    switch (index) {
                    case 0: addDialog.open(); break;
                    case 1: top100Model.reload(); break;
                    case 2: sortSheet.open(); break;
                    case 3: rankDialog.openRanking(); break;
                    case 4: aboutDialog.open(); break;
                    case 5: Qt.quit(); break;
                    }
                    navDrawer.close()
                }
            }
        }
    }

    header: ToolBar {
        RowLayout { anchors.fill: parent; spacing: 4
            ToolButton { text: "≡"; onClicked: navDrawer.open() }
            Label { text: qsTr("Top100"); font.bold: true; Layout.fillWidth: true }
            ToolButton { text: qsTr("Add"); onClicked: addDialog.open() }
            ToolButton { text: qsTr("Rank"); onClicked: rankDialog.openRanking() }
            ToolButton { text: qsTr("Refresh"); onClicked: top100Model.reload() }
        }
    }

    // Model selection state
    property int currentIndex: (listView.currentIndex >=0 ? listView.currentIndex : -1)
    property var currentMovie: (currentIndex >=0 ? top100Model.get(currentIndex) : ({}))

    // Main responsive layout: list over details (portrait) or side-by-side (landscape)
    StackLayout {
        id: stack
        anchors.fill: parent
        currentIndex: (window.width > window.height * 1.1) ? 0 : 0 // single primary page; details shown via sheet
        // Primary page
        Item {
            anchors.fill: parent
            ColumnLayout { anchors.fill: parent; spacing: 4
                // Sort + count row
                RowLayout {
                    Layout.margins: 6; spacing: 8
                    ComboBox {
                        id: sortCombo
                        model: [UiStrings_SortInsertion, UiStrings_SortByYear, UiStrings_SortAlpha, UiStrings_SortByRank, UiStrings_SortByScore]
                        Layout.fillWidth: true
                        Component.onCompleted: currentIndex = top100Model.sortOrder
                        onCurrentIndexChanged: top100Model.setSortOrder(currentIndex)
                    }
                    Label { text: listView.count + qsTr(" items") }
                }
                // Movie list
                ListView {
                    id: listView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: top100Model
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: contentRow.implicitHeight + 8
                        color: ListView.isCurrentItem ? Qt.rgba(0.2,0.4,0.8,0.15) : "transparent"
                        RowLayout { id: contentRow; anchors.fill: parent; anchors.margins: 6; spacing: 6
                            Label { text: (rank > 0 ? ("#"+rank+" ") : "") + title + " (" + year + ")"; elide: Text.ElideRight; Layout.fillWidth: true }
                        }
                        MouseArea { anchors.fill: parent; onClicked: listView.currentIndex = index; onDoubleClicked: detailsSheet.openSheet() }
                    }
                    Component.onCompleted: if (count>0) currentIndex = 0
                    onCountChanged: if (currentIndex < 0 && count>0) currentIndex = 0
                }
                Button { text: qsTr("Details"); enabled: currentIndex>=0; onClicked: detailsSheet.openSheet() }
            }
        }
    }

    // Details as a modal sheet for mobile friendliness
    Dialog {
        id: detailsSheet
        modal: true
        x: 0; y: parent.height*0.05
        width: parent.width
        height: parent.height*0.9
        title: (currentMovie.title || "") + (currentMovie.year ? (" ("+currentMovie.year+")") : "")
        standardButtons: Dialog.Close
        function openSheet() { if (currentIndex>=0) open() }
        contentItem: Flickable {
            anchors.fill: parent
            contentWidth: width
            contentHeight: col.implicitHeight
            ColumnLayout { id: col; width: parent.width; spacing: 8
                Item { Layout.fillWidth: true; Layout.preferredHeight: parent.height*0.35
                    Image {
                        id: poster
                        anchors.centerIn: parent
                        width: parent.width * Ui_PosterMaxWidthRatio
                        height: parent.height * Ui_PosterMaxHeightRatio
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        source: (currentMovie.posterUrl || "")
                        onStatusChanged: if (status===Image.Error) console.warn("Poster load failed", source)
                    }
                }
                Text { text: currentMovie.plotFull || ""; wrapMode: Text.WordWrap; width: parent.width - 16; Layout.margins: 8 }
                GridLayout { columns: 2; rowSpacing:4; columnSpacing:8; Layout.margins:8
                    Label { text: UiStrings_FieldDirector }
                    Label { text: currentMovie.director || ""; wrapMode: Text.WordWrap }
                    Label { text: UiStrings_FieldActors }
                    Label { text: currentMovie.actors ? currentMovie.actors.join(", ") : ""; wrapMode: Text.WordWrap }
                    Label { text: UiStrings_FieldGenres }
                    Label { text: currentMovie.genres ? currentMovie.genres.join(", ") : ""; wrapMode: Text.WordWrap }
                    Label { text: UiStrings_FieldRuntime }
                    Label { text: currentMovie.runtimeMinutes ? (currentMovie.runtimeMinutes + " min") : "" }
                }
                Button { text: qsTr("Open IMDb Page"); enabled: !!currentMovie.imdbID; onClicked: Qt.openUrlExternally("https://www.imdb.com/title/"+currentMovie.imdbID+"/") }
                RowLayout { spacing:8; Layout.margins:8
                    Button { text: qsTr("Update OMDb"); enabled: !!currentMovie.imdbID; onClicked: top100Model.updateFromOmdbByImdbId(currentMovie.imdbID) }
                    Button { text: qsTr("Post BlueSky"); enabled: currentIndex>=0; onClicked: top100Model.postToBlueSkyAsync(currentIndex) }
                    Button { text: qsTr("Post Mastodon"); enabled: currentIndex>=0; onClicked: top100Model.postToMastodonAsync(currentIndex) }
                }
            }
        }
    }

    // Ranking dialog (simplified mobile variant)
    Dialog {
        id: rankDialog
        modal: true; width: parent.width*0.95; height: parent.height*0.8; standardButtons: Dialog.Close
        title: qsTr("Rank Movies")
        function openRanking() { if (top100Model.count() >= 2) { leftIndex=-1; rightIndex=-1; pickTwo(); open(); } }
        property int leftIndex: -1
        property int rightIndex: -1
        function pickTwo() {
            var n = top100Model.count()
            if (n < 2) { leftIndex = rightIndex = -1; return }
            var a = Math.floor(Math.random()*n)
            var b = Math.floor(Math.random()*n); if (b===a) b = (a+1)%n
            leftIndex = a; rightIndex = b
        }
        function leftMovie() { return leftIndex>=0 ? top100Model.get(leftIndex) : ({}) }
        function rightMovie() { return rightIndex>=0 ? top100Model.get(rightIndex) : ({}) }
        function choose(which) {
            if (leftIndex<0 || rightIndex<0) return
            top100Model.recordPairwiseResult(leftIndex, rightIndex, which===0?1:0)
            pickTwo()
        }
        contentItem: ColumnLayout { spacing:8; anchors.fill: parent
            RowLayout { Layout.fillWidth: true; spacing:8
                Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color:"#20202020"; radius:4
                    ColumnLayout { anchors.fill: parent; anchors.margins:6; spacing:4
                        Label { text: (leftMovie().title||"") + (leftMovie().year?(" ("+leftMovie().year+")"):""); font.bold:true }
                        Text { text: leftMovie().plotFull||""; wrapMode: Text.WordWrap; Layout.fillWidth: true; Layout.preferredHeight: 80 }
                        Button { text: qsTr("Prefer this"); onClicked: choose(0) }
                    }
                }
                Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color:"#20202020"; radius:4
                    ColumnLayout { anchors.fill: parent; anchors.margins:6; spacing:4
                        Label { text: (rightMovie().title||"") + (rightMovie().year?(" ("+rightMovie().year+")"):""); font.bold:true }
                        Text { text: rightMovie().plotFull||""; wrapMode: Text.WordWrap; Layout.fillWidth: true; Layout.preferredHeight: 80 }
                        Button { text: qsTr("Prefer this"); onClicked: choose(1) }
                    }
                }
            }
            RowLayout { Layout.fillWidth: true; spacing:8
                Button { text: qsTr("Pass"); onClicked: pickTwo() }
                Button { text: qsTr("Close"); onClicked: rankDialog.close() }
            }
        }
    }

    // Add dialog (simplified mobile variant)
    Dialog {
        id: addDialog
        modal: true; width: parent.width*0.95; height: parent.height*0.85; standardButtons: Dialog.Close
        title: qsTr("Add Movie (OMDb)")
        property var results: []
        property var selected: null
        property bool searching: false
        property string statusMsg: ""
        contentItem: ColumnLayout { spacing: 6; anchors.fill: parent
            RowLayout { spacing:6; Layout.margins:6
                TextField { id: searchField; Layout.fillWidth: true; placeholderText: qsTr("title keyword"); enabled: !addDialog.searching; onAccepted: searchBtn.clicked() }
                Button { id: searchBtn; text: qsTr("Go"); enabled: !addDialog.searching; onClicked: {
                        var q = searchField.text; if (!q || q.trim().length===0) return
                        addDialog.searching = true; addDialog.statusMsg = qsTr("Searching…")
                        Qt.callLater(function(){
                            var list = top100Model.searchOmdb(q)
                            addDialog.searching = false
                            if (!list || list.length===0) { addDialog.results = []; addDialog.selected=null; addDialog.statusMsg = qsTr("No results"); return }
                            addDialog.results = list; addDialog.statusMsg = "";
                            // Auto-select first
                            resultsView.currentIndex = 0
                            var imdb = addDialog.results[0].imdbID
                            addDialog.selected = top100Model.omdbGetByIdMap(imdb)
                        })
                    } }
            }
            Label { text: addDialog.statusMsg; visible: addDialog.statusMsg.length>0; color: "#c33"; padding:4 }
            SplitView { Layout.fillWidth: true; Layout.fillHeight: true; orientation: Qt.Horizontal
                ListView { id: resultsView; model: addDialog.results; SplitView.preferredWidth: parent.width*0.45
                    delegate: ItemDelegate { width: parent.width; text: modelData.title + " ("+modelData.year+")"; onClicked: resultsView.currentIndex = index }
                    onCurrentIndexChanged: if (currentIndex>=0 && addDialog.results && addDialog.results.length>currentIndex) {
                        var imdb = addDialog.results[currentIndex].imdbID
                        addDialog.selected = top100Model.omdbGetByIdMap(imdb)
                    }
                }
                Flickable { SplitView.preferredWidth: parent.width*0.55; contentWidth: width; contentHeight: previewCol.implicitHeight
                    ColumnLayout { id: previewCol; width: parent.width; spacing:6
                        Label { text: addDialog.selected ? (addDialog.selected.title + (addDialog.selected.year?" ("+addDialog.selected.year+")":"")) : ""; font.bold:true }
                        Item { Layout.fillWidth: true; Layout.preferredHeight: 200
                            Image { anchors.centerIn: parent; width: parent.width*Ui_PosterMaxWidthRatio; height: parent.height*Ui_PosterMaxHeightRatio; fillMode: Image.PreserveAspectFit; asynchronous:true; source: (addDialog.selected && addDialog.selected.posterUrl)||""; onStatusChanged: if (status===Image.Error) console.warn("Poster load failed", source) }
                        }
                        Text { width: parent.width; wrapMode: Text.WordWrap; text: addDialog.selected ? (addDialog.selected.plotShort || addDialog.selected.plotFull || "") : "" }
                    }
                }
            }
            RowLayout { Layout.fillWidth: true; spacing:8; Layout.margins:6
                Button { text: qsTr("Add"); enabled: addDialog.selected && addDialog.selected.title; onClicked: {
                        if (resultsView.currentIndex>=0) {
                            var imdb = addDialog.results[resultsView.currentIndex].imdbID
                            if (top100Model.addMovieByImdbId(imdb)) { close(); } else { addDialog.statusMsg = qsTr("Add failed") }
                        }
                    } }
                Button { text: qsTr("Close"); onClicked: addDialog.close() }
            }
        }
    }

    Dialog { id: aboutDialog; modal: true; title: UiStrings_ActionAbout; standardButtons: Dialog.Close
        contentItem: Column { spacing:6; padding:8; Label { text: UiStrings_AboutText; wrapMode: Text.WordWrap } }
    }
}
