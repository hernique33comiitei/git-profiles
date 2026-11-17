import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.gitprofiles

Kirigami.ScrollablePage {
    id: page
    title: i18n("Apply")
    Kirigami.ColumnView.fillWidth: true

    ProfileModel {
        id: profiles
    }

    property string currentHost: ""
    property var result: ({})
    property bool hasPreview: false
    property string filter: "" // "", "change", "unchanged", "no-origin"
    property bool busy: false

    readonly property int dirCount: currentHost.length > 0 ? profiles.directoriesOf(currentHost).length : 0

    function indexOfHost(h) {
        for (var i = 0; i < profiles.count; i++)
            if (profiles.profileAt(i).host === h)
                return i
        return -1
    }

    function buildRepos() {
        var list = []
        var ds = profiles.directoriesOf(currentHost)
        for (var i = 0; i < ds.length; i++) {
            var found = RepoScanner.scan(ds[i].path, ds[i].children)
            for (var j = 0; j < found.length; j++)
                list.push({ path: found[j].path, origin: found[j].origin })
        }
        return list
    }

    function preview() {
        if (currentHost.length === 0) return
        busy = true
        result = RemoteSwitcher.apply(currentHost, buildRepos(), true)
        hasPreview = true
        filter = ""
        resultBanner.visible = false
        busy = false
    }

    function applyReal() {
        busy = true
        result = RemoteSwitcher.apply(currentHost, buildRepos(), false)
        busy = false
        resultBanner.text = (result.failed || 0) > 0
            ? i18n("%1 changed, %2 failed.", result.changed || 0, result.failed || 0)
            : i18n("Done — %1 repository(ies) updated.", result.changed || 0)
        resultBanner.type = (result.failed || 0) > 0 ? Kirigami.MessageType.Warning
                                                      : Kirigami.MessageType.Positive
        resultBanner.visible = true
    }

    function selectFirstIfNeeded() {
        var want = applicationWindow().selectedHost
        if (want && want.length > 0 && indexOfHost(want) >= 0)
            currentHost = want
        else if (currentHost.length === 0 && profiles.count > 0)
            currentHost = profiles.profileAt(0).host
        applyCombo.currentIndex = indexOfHost(currentHost)
        hasPreview = false
        result = ({})
    }

    Component.onCompleted: selectFirstIfNeeded()
    Connections {
        target: profiles
        function onCountChanged() { page.selectFirstIfNeeded() }
    }

    Kirigami.PromptDialog {
        id: confirmDialog
        title: i18n("Apply remote changes")
        subtitle: i18n("%1 repository(ies) will have their \"origin\" remote rewritten to \"%2\". This edits each repo's .git/config.",
            page.result.toChange || 0, page.currentHost)
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18n("Apply")
                icon.name: "dialog-ok-apply"
                onTriggered: { page.applyReal(); confirmDialog.close() }
            }
        ]
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            visible: profiles.count === 0
            icon.name: "user-identity"
            text: i18n("No profiles yet")
            explanation: i18n("Create a profile and attach directories first.")
        }

        RowLayout {
            Layout.fillWidth: true
            visible: profiles.count > 0
            Controls.Label { text: i18n("Profile:") }
            Controls.ComboBox {
                id: applyCombo
                Layout.fillWidth: true
                model: profiles
                textRole: "host"
                onActivated: {
                    page.currentHost = profiles.profileAt(currentIndex).host
                    applicationWindow().selectedHost = page.currentHost
                    page.hasPreview = false
                    page.result = ({})
                    resultBanner.visible = false
                }
            }
            Controls.ToolButton {
                icon.name: "view-refresh"
                display: Controls.AbstractButton.IconOnly
                enabled: page.dirCount > 0
                onClicked: page.preview()
                Controls.ToolTip.text: i18n("Rescan")
                Controls.ToolTip.visible: hovered
                Accessible.name: i18n("Rescan")
            }
        }

        Controls.BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: page.busy
            visible: page.busy
        }

        Kirigami.InlineMessage {
            id: resultBanner
            Layout.fillWidth: true
            visible: false
        }

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            visible: profiles.count > 0 && page.dirCount === 0
            icon.name: "folder"
            text: i18n("No directories for this profile")
            explanation: i18n("Attach directories to this profile before applying.")
            helpfulAction: Kirigami.Action {
                text: i18n("Go to Directories")
                icon.name: "folder"
                onTriggered: applicationWindow().navigateTo("directories")
            }
        }
        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            visible: profiles.count > 0 && page.dirCount > 0 && !page.hasPreview
            icon.name: "dialog-ok-apply"
            text: i18n("Ready to preview")
            explanation: i18n("Click Preview to see what will change.")
            helpfulAction: Kirigami.Action {
                text: i18n("Preview")
                icon.name: "view-visible-symbolic"
                onTriggered: page.preview()
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            visible: page.hasPreview

            Controls.Button {
                checkable: true
                checked: page.filter === ""
                text: i18n("All (%1)", page.result.total || 0)
                icon.name: "view-list-details-symbolic"
                onClicked: page.filter = ""
            }
            Controls.Button {
                checkable: true
                checked: page.filter === "change"
                text: i18n("Will change (%1)", page.result.toChange || 0)
                icon.name: "edit-rename"
                onClicked: page.filter = "change"
            }
            Controls.Button {
                checkable: true
                checked: page.filter === "unchanged"
                text: i18n("Unchanged (%1)", page.result.unchanged || 0)
                icon.name: "data-success"
                onClicked: page.filter = "unchanged"
            }
            Controls.Button {
                checkable: true
                checked: page.filter === "no-origin"
                text: i18n("No origin (%1)", page.result.noOrigin || 0)
                icon.name: "data-warning"
                onClicked: page.filter = "no-origin"
            }
        }

        Controls.Button {
            Layout.fillWidth: true
            visible: page.hasPreview && (page.result.toChange || 0) > 0 && page.result.dryRun === true
            text: i18n("Apply %1 change(s)", page.result.toChange || 0)
            icon.name: "dialog-ok-apply"
            highlighted: true
            onClicked: confirmDialog.open()
        }

        Repeater {
            model: page.result.log || []
            delegate: Kirigami.AbstractCard {
                required property var modelData
                Layout.fillWidth: true
                visible: page.filter === "" || modelData.status === page.filter
                contentItem: ColumnLayout {
                    spacing: 0
                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        Kirigami.Icon {
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                            source: modelData.status === "no-origin" ? "data-warning"
                                : modelData.status === "unchanged" ? "data-success"
                                : modelData.ok ? "data-success" : "data-error"
                        }
                        Kirigami.Heading {
                            level: 5
                            text: modelData.path
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                        }
                        Controls.Label {
                            opacity: 0.8
                            font: Kirigami.Theme.smallFont
                            text: modelData.status === "no-origin" ? i18n("no origin")
                                : modelData.status === "unchanged" ? i18n("unchanged")
                                : modelData.ok ? i18n("changed") : i18n("failed")
                        }
                    }
                    Controls.Label {
                        visible: modelData.status === "change"
                        text: modelData.oldUrl + "  →  " + modelData.newUrl
                        font.family: "monospace"
                        opacity: 0.8
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        visible: modelData.message && modelData.message.length > 0
                        text: modelData.message
                        color: Kirigami.Theme.negativeTextColor
                        font: Kirigami.Theme.smallFont
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
