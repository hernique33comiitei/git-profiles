import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.gitprofiles

Kirigami.ScrollablePage {
    id: page
    title: i18n("Profiles")
    Kirigami.ColumnView.fillWidth: true

    ProfileModel {
        id: profiles
    }

    Component {
        id: editorComponent
        ProfileEditor {}
    }

    function openEditor(originalHost, fields) {
        applicationWindow().pageStack.push(editorComponent, {
            model: profiles,
            originalHost: originalHost,
            fields: fields
        })
    }

    actions: [
        Kirigami.Action {
            text: i18n("New profile")
            icon.name: "list-add-symbolic"
            shortcut: "Ctrl+N"
            onTriggered: page.openEditor("", {})
        },
        Kirigami.Action {
            text: i18n("Reload")
            icon.name: "view-refresh"
            shortcut: "F5"
            onTriggered: profiles.reload()
        }
    ]

    header: Controls.Control {
        padding: Kirigami.Units.smallSpacing
        contentItem: Kirigami.SearchField {
            id: search
            placeholderText: i18n("Search profiles…")
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 8
        visible: profiles.count === 0
        icon.name: profiles.error ? "data-error" : "user-identity"
        text: profiles.error ? i18n("Could not read ~/.ssh/config") : i18n("No profiles yet")
        explanation: profiles.error ? profiles.error
                                    : i18n("Host blocks from %1 will show up here.", profiles.path)
        helpfulAction: profiles.error ? null : newAction
    }

    Kirigami.Action {
        id: newAction
        text: i18n("New profile")
        icon.name: "list-add-symbolic"
        onTriggered: page.openEditor("", {})
    }

    ListView {
        id: list
        model: profiles
        visible: profiles.count > 0
        currentIndex: -1

        delegate: Controls.ItemDelegate {
            id: delegate
            required property int index
            required property var model
            width: ListView.view.width

            readonly property string q: search.text.toLowerCase()
            readonly property bool matches: q.length === 0
                || model.host.toLowerCase().indexOf(q) >= 0
                || (model.hostName || "").toLowerCase().indexOf(q) >= 0

            height: matches ? implicitHeight : 0
            visible: matches
            clip: true
            onClicked: page.openEditor(model.host, profiles.profileAt(index))

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    source: "network-server-symbolic"
                    Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    Kirigami.Heading {
                        level: 4
                        text: delegate.model.host
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Controls.Label {
                        text: (delegate.model.user && delegate.model.user.length ? delegate.model.user : "git")
                            + "@" + (delegate.model.hostName || "")
                            + (delegate.model.port && delegate.model.port.length && delegate.model.port !== "22"
                               ? ":" + delegate.model.port : "")
                        visible: (delegate.model.hostName || "").length > 0
                        opacity: 0.7
                        font: Kirigami.Theme.smallFont
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing / 2
                        visible: (delegate.model.identityFile || "").length > 0
                        Layout.fillWidth: true
                        Kirigami.Icon {
                            visible: !delegate.model.identityExists
                            source: "data-warning"
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                            HoverHandler { id: keyHover }
                            Controls.ToolTip.text: i18n("Identity file not found")
                            Controls.ToolTip.visible: keyHover.hovered
                        }
                        Controls.Label {
                            text: i18n("key: %1", delegate.model.identityFile)
                            opacity: 0.6
                            font: Kirigami.Theme.smallFont
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }
                }

                Controls.Label {
                    visible: (delegate.model.provider || "").length > 0
                    text: delegate.model.provider === "github" ? "GitHub"
                        : delegate.model.provider === "gitea" ? "Gitea"
                        : delegate.model.provider === "gitlab" ? "GitLab" : delegate.model.provider
                    opacity: 0.7
                    font: Kirigami.Theme.smallFont
                }
                Controls.Label {
                    visible: (delegate.model.provider || "").length > 0
                    text: i18nc("badge: profile also has provider API access", "API")
                    color: Kirigami.Theme.highlightedTextColor
                    leftPadding: Kirigami.Units.smallSpacing
                    rightPadding: Kirigami.Units.smallSpacing
                    topPadding: Kirigami.Units.smallSpacing / 2
                    bottomPadding: Kirigami.Units.smallSpacing / 2
                    background: Rectangle {
                        radius: Kirigami.Units.cornerRadius
                        color: Kirigami.Theme.highlightColor
                    }
                }
            }
        }
    }
}
