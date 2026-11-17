import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.gitprofiles

Kirigami.ScrollablePage {
    id: page
    title: i18n("Dependencies")
    Kirigami.ColumnView.fillWidth: true

    ProfileModel {
        id: profiles
    }

    // Bumped on "Recheck" so the invokable-based bindings re-evaluate.
    property int tick: 0
    property var env: ProviderCli.environment()

    function recheck() {
        Requirements.recheck()
        env = ProviderCli.environment()
        tick++
    }

    actions: [
        Kirigami.Action {
            text: i18n("Recheck")
            icon.name: "view-refresh-symbolic"
            shortcut: "F5"
            onTriggered: page.recheck()
        }
    ]

    // level: "positive" | "warning" | "negative" | "neutral". The icon name is
    // mapped from the level in one place; Breeze status icons carry their colour.
    component StatusRow: Kirigami.AbstractCard {
        property string level: "neutral"
        property string title: ""
        property string detail: ""
        property string actionText: ""
        property string actionUrl: ""
        Layout.fillWidth: true
        contentItem: RowLayout {
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                implicitWidth: Kirigami.Units.iconSizes.smallMedium
                implicitHeight: Kirigami.Units.iconSizes.smallMedium
                source: level === "positive" ? "data-success"
                    : level === "warning" ? "data-warning"
                    : level === "negative" ? "data-error"
                    : "data-information"
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Kirigami.Heading {
                    level: 5
                    text: title
                }
                Controls.Label {
                    text: detail
                    visible: detail.length > 0
                    opacity: 0.8
                    font: Kirigami.Theme.smallFont
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }
            Controls.Button {
                visible: actionText.length > 0 && actionUrl.length > 0
                text: actionText
                icon.name: "help-contents-symbolic"
                onClicked: Qt.openUrlExternally(actionUrl)
            }
        }
    }

    function providerLabel(id) {
        return id === "github" ? "GitHub" : id === "gitea" ? "Gitea" : id === "gitlab" ? "GitLab" : id
    }
    function providerUsed(id) {
        return profiles.providersInUse().indexOf(id) >= 0
    }
    function providerLevel(id) {
        page.tick
        var p = Requirements.provider(id)
        if (!page.providerUsed(id) || !p.valid)
            return "neutral"
        return p.ok ? "positive" : "warning" // missing or outdated, but needed
    }
    function providerDetail(id) {
        page.tick
        var p = Requirements.provider(id)
        if (!page.providerUsed(id))
            return i18n("Optional — no profile uses %1.", providerLabel(id))
        if (p.ok)
            return i18n("v%1 — up to date.", p.version)
        if (p.installed)
            return i18n("Version %1 installed, requires ≥ %2.", p.version, p.required)
        return i18n("Not installed, requires ≥ %1.", p.required)
    }
    function providerAction(id) {
        page.tick
        var p = Requirements.provider(id)
        return (page.providerUsed(id) && p.valid && !p.ok)
            ? (p.installed ? i18n("How to update") : i18n("How to install")) : ""
    }
    function providerUrl(id) {
        var p = Requirements.provider(id)
        return p.valid ? p.url : ""
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            level: 3
            text: i18n("Required")
        }
        StatusRow {
            title: i18n("git")
            level: Requirements.gitInstalled ? "positive" : "negative"
            detail: Requirements.gitInstalled ? i18n("v%1 — up to date.", Requirements.gitVersion)
                                              : i18n("Not installed — required.")
            actionText: Requirements.gitInstalled ? "" : i18n("How to install")
            actionUrl: Requirements.gitUrl
        }

        Kirigami.Heading {
            level: 3
            text: i18n("Provider CLIs")
        }
        Repeater {
            model: ["github", "gitea", "gitlab"]
            delegate: StatusRow {
                required property string modelData
                title: i18n("%1 (%2)",
                    modelData === "github" ? "gh" : modelData === "gitea" ? "gt" : "glab",
                    page.providerLabel(modelData))
                level: page.providerLevel(modelData)
                detail: page.providerDetail(modelData)
                actionText: page.providerAction(modelData)
                actionUrl: page.providerUrl(modelData)
            }
        }

        Kirigami.Heading {
            level: 3
            text: i18n("Installed with the app")
        }
        StatusRow {
            title: i18n("direnv")
            level: (page.env.direnvOk && page.env.direnvHook) ? "positive" : "warning"
            detail: !page.env.direnvInstalled
                ? i18n("Comes with the app — run ./install.sh. Pinned ≥ %1.", page.env.direnvPin)
                : (page.env.direnvHook
                    ? i18n("v%1 (pinned ≥ %2), shell hook present.", page.env.direnvVersion, page.env.direnvPin)
                    : i18n("v%1 — add the hook to ~/.bashrc: eval \"$(direnv hook bash)\"", page.env.direnvVersion))
            actionText: page.env.direnvInstalled ? "" : i18n("How to install")
            actionUrl: page.env.direnvInstalled ? "" : page.env.direnvUrl
        }
        StatusRow {
            title: i18n("Keyring (secret-tool)")
            level: page.env.secretTool ? "positive" : "warning"
            detail: page.env.secretTool ? i18n("Available — tokens are stored securely in your keyring.")
                                        : i18n("Comes with the app (libsecret) — run ./install.sh.")
        }
    }
}
