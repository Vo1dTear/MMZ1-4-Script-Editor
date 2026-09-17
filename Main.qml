import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: window
    width: 1100
    height: 700
    minimumWidth: 800
    minimumHeight: 450
    title: (editorBackend.modified ? "* " : "") + (editorBackend.fileName || qsTr("Mega Man Zero 1–4 Script Editor"))
    property bool searchVisible: false
    property bool allowClose: false
    property string pendingAction: ""
    property url pendingUrl
    property var results: []

    function requestOpen(url) {
        pendingUrl = url
        pendingAction = "open"
        if (editorBackend.modified) unsaved.open()
        else completePending()
    }
    function completePending() {
        if (pendingAction === "close") {
            allowClose = true
            close()
        } else if (pendingUrl.toString().length) {
            editorBackend.openFile(pendingUrl)
        } else fileDialogController.open()
    }
    function refreshSearch() { results = editorBackend.search(searchField.text, regex.checked) }
    function toggleSearch() {
        searchVisible = !searchVisible
        if (searchVisible) searchField.forceActiveFocus()
    }
    onClosing: function(event) {
        if (editorBackend.modified && !allowClose) {
            event.accepted = false
            pendingAction = "close"
            unsaved.open()
        }
    }
    Connections {
        target: editorBackend
        function onTextChanged() {
            const cursor = textArea.cursorPosition
            textArea.text = editorBackend.text
            textArea.cursorPosition = Math.min(cursor, textArea.length)
        }
        function onSelectionChanged() { textArea.cursorPosition = 0 }
        function onStateChanged() { searchTimer.restart() }
        function onError(message) { errorDialog.text = message; errorDialog.open() }
        function onSaved(message) { window.showPassiveNotification(message) }
    }
    Connections {
        target: fileDialogController
        function onFileSelected(url) { editorBackend.openFile(url) }
    }
    Timer { id: searchTimer; interval: 180; onTriggered: window.refreshSearch() }
    Controls.Dialog {
        id: errorDialog
        width: Math.min(window.width - 40, 480)
        property alias text: errorLabel.text
        title: qsTr("Error")
        anchors.centerIn: parent
        modal: true
        standardButtons: Controls.Dialog.Ok
        contentItem: Controls.Label { id: errorLabel; wrapMode: Text.Wrap; width: 420 }
    }
    Controls.Dialog {
        id: unsaved
        width: Math.min(window.width - 40, 440)
        title: qsTr("Unsaved changes")
        anchors.centerIn: parent
        modal: true
        standardButtons: Controls.Dialog.Save | Controls.Dialog.Discard | Controls.Dialog.Cancel
        contentItem: Controls.Label { text: qsTr("Save all changes before continuing?") }
        onAccepted: { if (editorBackend.save(true)) window.completePending() }
        onDiscarded: window.completePending()
    }
    Shortcut { sequences: [StandardKey.Open]; onActivated: window.requestOpen("") }
    Shortcut { sequences: [StandardKey.Save]; onActivated: editorBackend.save(false) }
    Shortcut { sequence: "Ctrl+Shift+S"; onActivated: editorBackend.save(true) }
    Shortcut { sequence: "Ctrl+Shift+F"; onActivated: window.toggleSearch() }

    pageStack.initialPage: Kirigami.Page {
        title: qsTr("Script editor")
        actions: [
            Kirigami.Action { text: qsTr("Open TPL"); icon.source: "qrc:/icons/document-open.svg"; icon.color: Kirigami.Theme.textColor; onTriggered: window.requestOpen("") },
            Kirigami.Action { text: qsTr("Save script"); icon.source: "qrc:/icons/document-save.svg"; icon.color: Kirigami.Theme.textColor; enabled: editorBackend.currentIndex >= 0; onTriggered: editorBackend.save(false) },
            Kirigami.Action { text: qsTr("Save all"); icon.source: "qrc:/icons/document-save-all.svg"; icon.color: Kirigami.Theme.textColor; enabled: editorBackend.modified; onTriggered: editorBackend.save(true) },
            Kirigami.Action { text: qsTr("Search"); icon.source: "qrc:/icons/edit-find.svg"; icon.color: Kirigami.Theme.textColor; onTriggered: window.toggleSearch() }
        ]
        DropArea {
            anchors.fill: parent
            onDropped: function(drop) {
                if (drop.hasUrls && drop.urls.length === 1) {
                    window.requestOpen(drop.urls[0])
                    drop.acceptProposedAction()
                }
            }
        }
        ColumnLayout {
            anchors.fill: parent
            Controls.SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Controls.ScrollView {
                    Controls.SplitView.preferredWidth: 220
                    Controls.SplitView.minimumWidth: 150
                    ListView {
                        id: scriptsList
                        objectName: "scriptsList"
                        clip: true
                        activeFocusOnTab: true
                        keyNavigationEnabled: false
                        // Keep delegates and scroll position when only script
                        // metadata changes (editing, undo/redo or saving).
                        model: editorBackend.scripts.length
                        currentIndex: editorBackend.currentIndex
                        onCurrentIndexChanged: {
                            if (currentIndex >= 0)
                                positionViewAtIndex(currentIndex, ListView.Contain)
                        }
                        Keys.onUpPressed: editorBackend.selectScript(currentIndex - 1)
                        Keys.onDownPressed: editorBackend.selectScript(currentIndex + 1)
                        MouseArea {
                            parent: scriptsList
                            anchors.fill: parent
                            z: 1
                            acceptedButtons: Qt.LeftButton
                            preventStealing: true

                            function selectAtPointer(mouse) {
                                if (mouse.x < 0 || mouse.x >= width || mouse.y < 0 || mouse.y >= height)
                                    return
                                const point = mapToItem(scriptsList.contentItem, mouse.x, mouse.y)
                                const index = scriptsList.indexAt(point.x, point.y)
                                if (index >= 0)
                                    editorBackend.selectScript(index)
                            }
                            onPressed: function(mouse) {
                                scriptsList.forceActiveFocus()
                                selectAtPointer(mouse)
                            }
                            onPositionChanged: function(mouse) {
                                if (pressed)
                                    selectAtPointer(mouse)
                            }
                            onWheel: function(wheel) { wheel.accepted = false }
                        }
                        delegate: Controls.ItemDelegate {
                            required property int index
                            readonly property var modelData: editorBackend.scripts[index]
                            objectName: "scriptRow" + index
                            width: ListView.view.width
                            text: (modelData.modified ? "* " : "") + modelData.label
                            highlighted: index === editorBackend.currentIndex
                            contentItem: Controls.Label {
                                text: (modelData.modified ? "* " : "") + modelData.label
                                font.bold: true
                                color: modelData.modified ? "#d97706" : (modelData.saved ? "#2e7d32" : palette.text)
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            onClicked: {
                                scriptsList.forceActiveFocus()
                                editorBackend.selectScript(index)
                            }
                        }
                    }
                }
                ColumnLayout {
                    Controls.SplitView.fillWidth: true
                    Controls.SplitView.minimumWidth: 300
                    Controls.Label {
                        text: editorBackend.currentIndex >= 0 ? editorBackend.scripts[editorBackend.currentIndex].label : qsTr("Open or drop a TPL file to begin")
                        font.bold: true
                    }
                    Controls.ScrollView {
                        id: editorScroll
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        Controls.TextArea {
                            id: textArea
                            objectName: "scriptTextArea"
                            readOnly: editorBackend.currentIndex < 0
                            textFormat: TextEdit.PlainText
                            wrapMode: TextEdit.NoWrap
                            font.family: editorFontFamily
                            font.pointSize: 11
                            selectByMouse: true
                            persistentSelection: true
                            leftPadding: 60
                            // Use the document layout, which can round line spacing or
                            // use fallback fonts differently from FontMetrics.
                            readonly property var lineStarts: {
                                const starts = [0]
                                for (let i = 0; i < text.length; ++i) {
                                    if (text[i] === "\n" || text[i] === "\u2028" || text[i] === "\u2029")
                                        starts.push(i + 1)
                                }
                                return starts
                            }
                            onTextChanged: editorBackend.editText(text)
                            Keys.onPressed: function(event) {
                                if (event.matches(StandardKey.Undo)) { editorBackend.undo(); event.accepted = true }
                                else if (event.matches(StandardKey.Redo)) { editorBackend.redo(); event.accepted = true }
                            }
                            Rectangle {
                                x: textArea.leftPadding
                                y: textArea.cursorRectangle.y
                                width: Math.max(0, textArea.width - textArea.leftPadding)
                                height: textArea.cursorRectangle.height
                                color: Kirigami.Theme.highlightColor
                                opacity: 0.08
                                z: -1
                            }
                            Repeater {
                                model: textArea.lineStarts
                                Text {
                                    required property int index
                                    required property int modelData
                                    objectName: "lineNumber" + index
                                    readonly property rect lineRectangle: {
                                        // positionToRectangle is a method: explicitly
                                        // track changes that can move the text layout.
                                        textArea.text
                                        textArea.font
                                        textArea.contentHeight
                                        textArea.topPadding
                                        // Old delegates can update before the repeater
                                        // removes them when the document gets shorter.
                                        if (modelData < 0 || modelData > textArea.length)
                                            return Qt.rect(0, 0, 0, 0)
                                        return textArea.positionToRectangle(modelData)
                                    }
                                    x: 0
                                    y: lineRectangle.y
                                    width: 48
                                    height: lineRectangle.height
                                    horizontalAlignment: Text.AlignRight
                                    text: index + 1
                                    font: textArea.font
                                    color: Kirigami.Theme.disabledTextColor
                                }
                            }
                        }
                    }
                    RowLayout {
                        Controls.Button { text: qsTr("Undo"); enabled: editorBackend.canUndo; onClicked: editorBackend.undo() }
                        Controls.Button { text: qsTr("Redo"); enabled: editorBackend.canRedo; onClicked: editorBackend.redo() }
                        Item { Layout.fillWidth: true }
                        Controls.Label {
                            text: {
                                if (textArea.selectedText.length) return qsTr("Selected: %1").arg(textArea.selectedText.replace(/\t/g, "").length)
                                const before = textArea.text.substring(0, textArea.cursorPosition)
                                return qsTr("Characters: %1").arg(before.substring(before.lastIndexOf("\n") + 1).replace(/\t/g, "").length)
                            }
                        }
                    }
                }
                ColumnLayout {
                    visible: window.searchVisible
                    Controls.SplitView.preferredWidth: 300
                    Controls.SplitView.minimumWidth: 180
                    Controls.TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search all scripts…")
                        onTextChanged: searchTimer.restart()
                    }
                    Controls.CheckBox { id: regex; text: qsTr("Use regular expressions"); onToggled: searchTimer.restart() }
                    Controls.Label { text: qsTr("%1 matches").arg(window.results.length) }
                    Controls.ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        ListView {
                            clip: true
                            model: window.results
                            delegate: Controls.ItemDelegate {
                                required property var modelData
                                width: ListView.view.width
                                text: modelData.label
                                onClicked: {
                                    editorBackend.selectScript(modelData.scriptIndex)
                                    textArea.forceActiveFocus()
                                    textArea.select(modelData.position, modelData.position + modelData.length)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
