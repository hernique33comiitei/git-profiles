import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.gitprofiles

Kirigami.ScrollablePage {
    id: editor

    property var model            // ProfileModel
    property string originalHost: ""
    property var fields: ({})

    readonly property bool isNew: originalHost.length === 0
    readonly property var providerIds: ["", "github", "gitea", "gitlab"]
    readonly property var providerLabels: [i18n("None (SSH only)"), "GitHub", "Gitea", "GitLab"]
    readonly property string currentProvider: providerIds[providerCombo.currentIndex]

    property bool loaded: false
    property bool dirty: false

    readonly property string hostError: {
        if (!editor.model)
            return ""
        var h = hostField.text.trim()
        if (h.length === 0)
            return i18n("A Host alias is required.")
        if (editor.model.hostTaken(h, editor.originalHost))
            return i18n("A profile named \"%1\" already exists.", h)
        return ""
    }
    readonly property string hostNameError: hostNameField.text.trim().length === 0
        ? i18n("HostName is required.") : ""
    readonly property bool canSave: dirty && hostError.length === 0 && hostNameError.length === 0

    title: isNew ? i18n("New profile") : i18n("Edit %1", originalHost)

    function markDirty() { if (loaded) dirty = true }

    function fieldsNow() {
        return {
            host: hostField.text.trim(),
            hostName: hostNameField.text.trim(),
            user: userField.text.trim(),
            port: portSpin.value === 22 ? "" : String(portSpin.value),
            identityFile: identityField.text.trim(),
            provider: editor.currentProvider,
            apiUrl: apiUrlField.text.trim(),
            owner: ownerField.text.trim(),
            ghConfig: ghConfigField.text.trim()
        }
    }

    function updateEnvrc() {
        envrcArea.text = (editor.currentProvider.length > 0) ? ProviderCli.envrc(editor.fieldsNow()) : ""
    }
    function refreshTokenStatus() {
        tokenStatus.hasToken = (hostField.text.trim().length > 0) && Secrets.hasToken(hostField.text.trim())
    }

    function save() {
        if (!editor.canSave)
            return
        var err = editor.model.saveProfile(editor.originalHost, editor.fieldsNow())
        if (err.length > 0) {
            errorBanner.text = err
            errorBanner.visible = true
        } else {
            applicationWindow().pageStack.pop()
        }
    }

    actions: [
        Kirigami.Action {
            text: i18n("Save")
            icon.name: "document-save"
            shortcut: "Ctrl+S"
            enabled: editor.canSave
            onTriggered: editor.save()
        },
        Kirigami.Action {
            text: i18n("Cancel")
            icon.name: "dialog-cancel"
            onTriggered: applicationWindow().pageStack.pop()
        },
        Kirigami.Action {
            text: i18n("Test connection")
            icon.name: "network-server-symbolic"
            enabled: !SshTester.running && hostField.text.trim().length > 0
            onTriggered: {
                testBanner.visible = false
                SshTester.test(hostField.text.trim())
            }
        },
        Kirigami.Action {
            text: i18n("Delete")
            icon.name: "edit-delete"
            visible: !editor.isNew
            onTriggered: deleteDialog.open()
        }
    ]

    FileDialog {
        id: keyDialog
        title: i18n("Choose an identity file")
        currentFolder: "file://" + ProviderCli.homeDir() + "/.ssh"
        onAccepted: {
            var p = decodeURIComponent(("" + selectedFile).replace("file://", ""))
            var home = ProviderCli.homeDir()
            if (p.indexOf(home + "/") === 0)
                p = "~" + p.substring(home.length)
            identityField.text = p
            editor.markDirty()
            editor.updateEnvrc()
        }
    }

    Connections {
        target: SshTester
        function onFinished(level, message) {
            testBanner.text = level === "success" ? i18n("Authenticated successfully. %1", message)
                : level === "authfail" ? i18n("Authentication failed. %1", message)
                : level === "unreachable" ? i18n("Host unreachable. %1", message)
                : (message.length > 0 ? message : i18n("Finished with no clear result."))
            testBanner.type = level === "success" ? Kirigami.MessageType.Positive
                : level === "unknown" ? Kirigami.MessageType.Information
                : level === "authfail" ? Kirigami.MessageType.Error
                : Kirigami.MessageType.Warning
            testBanner.visible = true
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: SshTester.running
            Controls.BusyIndicator {
                running: SshTester.running
                implicitWidth: Kirigami.Units.iconSizes.medium
                implicitHeight: Kirigami.Units.iconSizes.medium
            }
            Controls.Label { text: i18n("Testing connection…") }
        }
        Kirigami.InlineMessage {
            id: testBanner
            Layout.fillWidth: true
            visible: false
        }

        // One FormLayout keeps every labelled section centered and aligned.
        // wideMode keeps labels beside the fields instead of stacking them on top.
        Kirigami.FormLayout {
            wideMode: true
            Layout.fillWidth: true

            Kirigami.InlineMessage {
                id: errorBanner
                Layout.fillWidth: true
                type: Kirigami.MessageType.Error
                visible: false
            }

            Kirigami.Separator {
                Kirigami.FormData.label: i18n("SSH connection")
                Kirigami.FormData.isSection: true
            }

            Controls.TextField {
                id: hostField
                Kirigami.FormData.label: i18n("Host alias")
                placeholderText: "github-band"
                onTextChanged: { editor.markDirty(); editor.refreshTokenStatus() }
            }
            Controls.Label {
                visible: editor.hostError.length > 0
                text: editor.hostError
                color: Kirigami.Theme.negativeTextColor
                font: Kirigami.Theme.smallFont
            }
            Controls.TextField {
                id: hostNameField
                Kirigami.FormData.label: i18n("HostName")
                placeholderText: "github.com"
                onTextChanged: { editor.markDirty(); editor.updateEnvrc() }
            }
            Controls.Label {
                visible: editor.hostNameError.length > 0
                text: editor.hostNameError
                color: Kirigami.Theme.negativeTextColor
                font: Kirigami.Theme.smallFont
            }
            Controls.TextField {
                id: userField
                Kirigami.FormData.label: i18n("User")
                placeholderText: "git"
                onTextChanged: editor.markDirty()
            }
            Controls.SpinBox {
                id: portSpin
                Kirigami.FormData.label: i18n("Port")
                from: 1
                to: 65535
                editable: true
                value: 22
                onValueModified: editor.markDirty()
            }
            RowLayout {
                Kirigami.FormData.label: i18n("IdentityFile")
                Controls.TextField {
                    id: identityField
                    Layout.fillWidth: true
                    placeholderText: "~/.ssh/id_ed25519"
                    onTextChanged: editor.markDirty()
                }
                Controls.Button {
                    icon.name: "document-open-symbolic"
                    display: Controls.AbstractButton.IconOnly
                    onClicked: keyDialog.open()
                    Controls.ToolTip.text: i18n("Browse…")
                    Controls.ToolTip.visible: hovered
                    Accessible.name: i18n("Browse for an identity file")
                }
            }

            Kirigami.Separator {
                Kirigami.FormData.label: i18n("Provider")
                Kirigami.FormData.isSection: true
            }

            Controls.ComboBox {
                id: providerCombo
                Kirigami.FormData.label: i18n("Type")
                model: editor.providerLabels
                onActivated: { editor.markDirty(); editor.updateEnvrc() }
            }
            Controls.TextField {
                id: apiUrlField
                Kirigami.FormData.label: i18n("API URL")
                placeholderText: "https://git.example.com"
                visible: editor.currentProvider.length > 0
                onTextChanged: { editor.markDirty(); editor.updateEnvrc() }
            }
            Controls.TextField {
                id: ownerField
                Kirigami.FormData.label: i18n("Owner / account")
                visible: editor.currentProvider.length > 0
                onTextChanged: { editor.markDirty(); editor.updateEnvrc() }
            }
            Controls.TextField {
                id: ghConfigField
                Kirigami.FormData.label: i18n("gh config dir")
                placeholderText: "~/.config/gh-profiles/..."
                visible: editor.currentProvider === "github"
                onTextChanged: { editor.markDirty(); editor.updateEnvrc() }
            }

            Kirigami.Separator {
                Kirigami.FormData.label: i18n("Credentials")
                Kirigami.FormData.isSection: true
                visible: editor.currentProvider.length > 0
            }
            Kirigami.InlineMessage {
                id: credBanner
                Layout.fillWidth: true
                visible: false
            }
            RowLayout {
                Kirigami.FormData.label: i18n("Token")
                visible: editor.currentProvider.length > 0
                Controls.TextField {
                    id: tokenField
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    placeholderText: i18n("Paste a personal access token")
                }
                Controls.Button {
                    text: i18n("Save credentials")
                    icon.name: "document-encrypted-symbolic"
                    enabled: tokenField.text.length > 0
                    onClicked: {
                        var err = ProviderCli.setupCredential(editor.fieldsNow(), tokenField.text)
                        if (err.length > 0) {
                            credBanner.text = err
                            credBanner.type = Kirigami.MessageType.Error
                        } else {
                            tokenField.text = ""
                            credBanner.text = i18n("Credentials saved to the keyring.")
                            credBanner.type = Kirigami.MessageType.Positive
                            editor.refreshTokenStatus()
                        }
                        credBanner.visible = true
                    }
                }
            }
            Controls.Label {
                id: tokenStatus
                property bool hasToken: false
                visible: editor.currentProvider.length > 0
                text: hasToken ? i18n("Token stored in the keyring") : i18n("No token stored yet")
                opacity: 0.7
                font: Kirigami.Theme.smallFont
            }

            Kirigami.Separator {
                Kirigami.FormData.label: i18n(".envrc (direnv) preview")
                Kirigami.FormData.isSection: true
                visible: editor.currentProvider.length > 0
            }
        }

        // The preview has no label, so center it rather than leaving a gap where
        // the label column would be.
        Controls.TextArea {
            id: envrcArea
            visible: editor.currentProvider.length > 0
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(editor.width - Kirigami.Units.gridUnit * 3, Kirigami.Units.gridUnit * 32)
            readOnly: true
            font.family: "monospace"
            wrapMode: TextEdit.Wrap
        }
    }

    Kirigami.PromptDialog {
        id: deleteDialog
        title: i18n("Delete profile")
        subtitle: i18n("Remove \"%1\" from ~/.ssh/config? A timestamped backup is kept.", editor.originalHost)
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18n("Delete")
                icon.name: "edit-delete"
                onTriggered: {
                    var e = editor.model.removeProfile(editor.originalHost)
                    deleteDialog.close()
                    if (e.length > 0) {
                        errorBanner.text = e
                        errorBanner.visible = true
                    } else {
                        applicationWindow().pageStack.pop()
                    }
                }
            }
        ]
    }

    Component.onCompleted: {
        hostField.text = fields.host || ""
        hostNameField.text = fields.hostName || ""
        userField.text = fields.user || ""
        portSpin.value = (fields.port && fields.port.length) ? parseInt(fields.port) : 22
        identityField.text = fields.identityFile || ""
        apiUrlField.text = fields.apiUrl || ""
        ownerField.text = fields.owner || ""
        ghConfigField.text = fields.ghConfig || ""
        providerCombo.currentIndex = Math.max(0, editor.providerIds.indexOf(fields.provider || ""))
        editor.updateEnvrc()
        editor.refreshTokenStatus()
        editor.loaded = true
    }
}
