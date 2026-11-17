import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.gitprofiles

Kirigami.ApplicationWindow {
    id: root

    title: "Git Profiles"

    width: Kirigami.Units.gridUnit * 50
    height: Kirigami.Units.gridUnit * 34
    minimumWidth: Kirigami.Units.gridUnit * 34
    minimumHeight: Kirigami.Units.gridUnit * 24

    // Hard gate: without git, the app shows only the warning page — nothing else.
    readonly property bool gitOk: Requirements.gitInstalled

    // Profile selected by the user, kept for the whole session.
    property string selectedHost: ""

    // Always one full-width page at a time, so the editor opens full width
    // instead of a cramped side column that clips the form.
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.NoNavigationButtons

    function navigate(comp) {
        pageStack.clear()
        pageStack.push(comp)
    }

    function navigateTo(name) {
        if (name === "profiles") navigate(profilesPage)
        else if (name === "directories") navigate(directoriesPage)
        else if (name === "apply") navigate(applyPage)
        else if (name === "settings") navigate(settingsPage)
    }

    globalDrawer: Kirigami.GlobalDrawer {
        id: appDrawer
        modal: false
        enabled: root.gitOk

        // Just the app icon at the top, aligned to the standard side gutter.
        header: Item {
            implicitHeight: Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.largeSpacing * 2
            Kirigami.Icon {
                source: "qrc:/data/icons/gitprofiles.svg"
                width: Kirigami.Units.iconSizes.smallMedium
                height: width
                anchors.left: parent.left
                anchors.leftMargin: Kirigami.Units.largeSpacing
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        actions: [
            Kirigami.Action {
                text: i18n("Profiles")
                icon.name: "user-identity"
                onTriggered: root.navigate(profilesPage)
            },
            Kirigami.Action {
                text: i18n("Directories")
                icon.name: "folder"
                onTriggered: root.navigate(directoriesPage)
            },
            Kirigami.Action {
                text: i18n("Apply")
                icon.name: "dialog-ok-apply"
                onTriggered: root.navigate(applyPage)
            },
            Kirigami.Action {
                text: i18n("Settings")
                icon.name: "configure"
                onTriggered: root.navigate(settingsPage)
            }
        ]
    }

    pageStack.initialPage: root.gitOk ? profilesPage : gitMissingPage

    Component { id: gitMissingPage; GitMissingPage {} }
    Component { id: profilesPage; ProfilesPage {} }
    Component { id: directoriesPage; DirectoriesPage {} }
    Component { id: applyPage; ApplyPage {} }
    Component { id: settingsPage; SettingsPage {} }
}
