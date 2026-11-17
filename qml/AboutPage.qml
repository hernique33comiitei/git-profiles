import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.gitprofiles

Kirigami.ScrollablePage {
    id: page
    title: i18n("About")
    Kirigami.ColumnView.fillWidth: true

    ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.largeSpacing
            source: "gitprofiles"
            implicitWidth: Kirigami.Units.iconSizes.huge
            implicitHeight: Kirigami.Units.iconSizes.huge
        }
        Kirigami.Heading {
            Layout.alignment: Qt.AlignHCenter
            level: 1
            text: "Git Profiles"
        }
        Controls.Label {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Version %1", Config.version)
            opacity: 0.8
        }
        Controls.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: i18n("Organize your git repositories by SSH profile")
            opacity: 0.9
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.smallSpacing
            Controls.Button {
                text: i18n("Source code")
                icon.name: "internet-services-symbolic"
                onClicked: Qt.openUrlExternally("https://github.com/hernique33comiitei")
            }
            Controls.Button {
                text: "LinkedIn"
                icon.name: "internet-services-symbolic"
                onClicked: Qt.openUrlExternally("https://www.linkedin.com/in/henrique-oliveira-1639a924a/")
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        Kirigami.FormLayout {
            Layout.fillWidth: true
            Controls.Label {
                Kirigami.FormData.label: i18n("Author")
                text: "Henrique Oliveira"
            }
            Controls.Label {
                Kirigami.FormData.label: i18n("License")
                text: "MIT"
            }
        }
    }
}
