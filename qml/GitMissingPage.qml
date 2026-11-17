import QtQuick
import org.kde.kirigami as Kirigami
import org.gitprofiles

Kirigami.Page {
    title: i18n("Git required")

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 8
        icon.name: "data-error"
        text: i18n("Git is not installed")
        explanation: i18n("Git Profiles needs Git to manage your repositories. Install the latest version of Git, then reopen this app.")
        helpfulAction: Kirigami.Action {
            text: i18n("Download Git")
            icon.name: "internet-services-symbolic"
            onTriggered: Qt.openUrlExternally(Requirements.gitUrl)
        }
    }
}
