import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.gitprofiles

Kirigami.ScrollablePage {
    id: page
    title: i18n("Directories")
    Kirigami.ColumnView.fillWidth: true

    ProfileModel {
        id: profiles
    }

    property string currentHost: ""
    property var dirs: []
    property int tick: 0 // bump to recompute per-item counts/status
    readonly property var env: ProviderCli.environment()
    readonly property var profile: currentHost.length > 0 ? profiles.profileByHost(currentHost) : ({})
    readonly property bool hasProvider: profile.provider !== undefined && profile.provider.length > 0

    function stripFileUrl(u) {
        var s = "" + u
        if (s.indexOf("file://") === 0)
            s = s.substring(7)
        return decodeURIComponent(s)
    }

    function refresh() {
        dirs = currentHost.length > 0 ? profiles.directoriesOf(currentHost) : []
        tick++
        updateOverlapWarning()
    }

    function updateOverlapWarning() {
        var shared = []
        for (var i = 0; i < dirs.length; i++) {
            for (var r = 0; r < profiles.count; r++) {
                var other = profiles.profileAt(r).host
                if (other === currentHost) continue
                var od = profiles.directoriesOf(other)
                for (var j = 0; j < od.length; j++) {
                    if (od[j].path === dirs[i].path && shared.indexOf(dirs[i].path) < 0)
                        shared.push(dirs[i].path)
                }
            }
        }
        if (shared.length > 0) {
            overlapBanner.text = i18n("Also attached to another profile: %1", shared.join(", "))
            overlapBanner.visible = true
        } else {
            overlapBanner.visible = false
        }
    }

    function indexOfHost(h) {
        for (var i = 0; i < profiles.count; i++)
            if (profiles.profileAt(i).host === h)
                return i
        return -1
    }

    function selectFirstIfNeeded() {
        var want = applicationWindow().selectedHost
        if (want && want.length > 0 && indexOfHost(want) >= 0)
            currentHost = want
        else if (currentHost.length === 0 && profiles.count > 0)
            currentHost = profiles.profileAt(0).host
        dirCombo.currentIndex = indexOfHost(currentHost)
        refresh()
    }

    function generateOne(path) {
        var e = ProviderCli.writeEnvrc(path, page.profile)
        if (e.length > 0) {
            errorBanner.text = e
            errorBanner.type = Kirigami.MessageType.Error
            errorBanner.visible = true
        } else {
            tick++
            applicationWindow().showPassiveNotification(
                i18n(".envrc written to %1", path), "short")
        }
    }

    Component.onCompleted: selectFirstIfNeeded()
    Connections {
        target: profiles
        function onCountChanged() { page.selectFirstIfNeeded() }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Generate all .envrc")
            icon.name: "document-save-all-symbolic"
            enabled: page.hasProvider && page.dirs.length > 0
            onTriggered: {
                var ds = profiles.directoriesOf(page.currentHost)
                for (var i = 0; i < ds.length; i++)
                    ProviderCli.writeEnvrc(ds[i].path, page.profile)
                page.tick++
                applicationWindow().showPassiveNotification(
                    i18n(".envrc written to %1 directory(ies)", ds.length), "short")
            }
            Controls.ToolTip.text: i18n("Writes a managed block exporting the provider credentials into each directory's .envrc")
        }
    ]

    FolderDialog {
        id: folderDialog
        title: i18n("Choose a directory to attach")
        onAccepted: {
            var err = profiles.attachDirectory(page.currentHost, page.stripFileUrl(selectedFolder), false)
            if (err.length > 0) {
                errorBanner.text = err
                errorBanner.type = Kirigami.MessageType.Error
                errorBanner.visible = true
            } else {
                page.refresh()
            }
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            id: errorBanner
            Layout.fillWidth: true
            type: Kirigami.MessageType.Error
            visible: false
        }
        Kirigami.InlineMessage {
            id: overlapBanner
            Layout.fillWidth: true
            type: Kirigami.MessageType.Warning
            visible: false
        }

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            visible: profiles.count === 0
            icon.name: "user-identity"
            text: i18n("No profiles yet")
            explanation: i18n("Create a profile on the Profiles page first.")
        }

        RowLayout {
            Layout.fillWidth: true
            visible: profiles.count > 0
            Controls.Label { text: i18n("Profile:") }
            Controls.ComboBox {
                id: dirCombo
                Layout.fillWidth: true
                model: profiles
                textRole: "host"
                onActivated: {
                    page.currentHost = profiles.profileAt(currentIndex).host
                    applicationWindow().selectedHost = page.currentHost
                    page.refresh()
                }
            }
            Controls.Button {
                text: i18n("Add directory…")
                icon.name: "list-add-symbolic"
                enabled: page.currentHost.length > 0
                onClicked: folderDialog.open()
            }
        }

        // Empty state for the directory list
        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            visible: profiles.count > 0 && page.dirs.length === 0
            icon.name: "folder"
            text: i18n("No directories attached to this profile")
            explanation: i18n("Attach a directory to rewrite the git remotes of the repositories in it.")
            helpfulAction: Kirigami.Action {
                text: i18n("Add directory…")
                icon.name: "list-add-symbolic"
                enabled: page.currentHost.length > 0
                onTriggered: folderDialog.open()
            }
        }

        Repeater {
            model: page.dirs
            delegate: Kirigami.AbstractCard {
                id: dirCard
                required property var modelData
                Layout.fillWidth: true

                readonly property int repoCount: (page.tick, RepoScanner.countRepos(modelData.path, modelData.children))
                readonly property string envStatus: (page.tick, page.hasProvider
                    ? ProviderCli.envrcStatus(modelData.path, page.profile) : "")

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        Kirigami.Icon {
                            source: "folder-symbolic"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                        }
                        Controls.Label {
                            text: dirCard.modelData.path
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                        }
                        Controls.Label {
                            text: i18np("%1 repository", "%1 repositories", dirCard.repoCount)
                            opacity: 0.7
                            font: Kirigami.Theme.smallFont
                        }
                        Controls.ToolButton {
                            icon.name: "edit-delete-symbolic"
                            display: Controls.AbstractButton.IconOnly
                            Controls.ToolTip.text: i18n("Remove from profile")
                            Controls.ToolTip.visible: hovered
                            Accessible.name: i18n("Remove from profile")
                            onClicked: {
                                var p = dirCard.modelData.path
                                var ch = dirCard.modelData.children
                                var e = profiles.detachDirectory(page.currentHost, p)
                                if (e.length > 0) {
                                    errorBanner.text = e
                                    errorBanner.type = Kirigami.MessageType.Error
                                    errorBanner.visible = true
                                } else {
                                    page.refresh()
                                    applicationWindow().showPassiveNotification(
                                        i18n("Removed %1", p), "short", i18n("Undo"),
                                        function() {
                                            profiles.attachDirectory(page.currentHost, p, ch)
                                            page.refresh()
                                        })
                                }
                            }
                        }
                    }

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        Controls.Switch {
                            text: i18n("Include direct subfolders")
                            checked: dirCard.modelData.children
                            onToggled: {
                                var e = profiles.setDirChildren(page.currentHost, dirCard.modelData.path, checked)
                                if (e.length > 0) {
                                    errorBanner.text = e
                                    errorBanner.type = Kirigami.MessageType.Error
                                    errorBanner.visible = true
                                } else {
                                    page.refresh()
                                }
                            }
                            Controls.ToolTip.text: i18n("Also includes %1/* (one level only, not deeper)", dirCard.modelData.path)
                            Controls.ToolTip.visible: hovered
                        }

                        Item { Layout.fillWidth: true }

                        // .envrc status + per-item generate
                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing / 2
                            visible: page.hasProvider
                            Kirigami.Icon {
                                implicitWidth: Kirigami.Units.iconSizes.small
                                implicitHeight: Kirigami.Units.iconSizes.small
                                source: dirCard.envStatus === "ok" ? "data-success"
                                    : dirCard.envStatus === "outdated" ? "data-warning"
                                    : "data-information"
                            }
                            Controls.Label {
                                text: dirCard.envStatus === "ok" ? i18n(".envrc up to date")
                                    : dirCard.envStatus === "outdated" ? i18n(".envrc outdated")
                                    : i18n("no .envrc")
                                opacity: 0.7
                                font: Kirigami.Theme.smallFont
                            }
                            Controls.ToolButton {
                                icon.name: "document-save-symbolic"
                                display: Controls.AbstractButton.IconOnly
                                visible: dirCard.envStatus !== "ok"
                                Controls.ToolTip.text: i18n("Generate .envrc here")
                                Controls.ToolTip.visible: hovered
                                Accessible.name: i18n("Generate .envrc here")
                                onClicked: page.generateOne(dirCard.modelData.path)
                            }
                        }
                    }
                }
            }
        }
    }
}
