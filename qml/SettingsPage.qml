import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.gitprofiles

Kirigami.ScrollablePage {
    id: page
    title: i18n("Settings")
    Kirigami.ColumnView.fillWidth: true

    readonly property var langCodes: ["", "en", "pt_BR"]

    Component { id: depsComp; DependenciesPage {} }
    Component { id: aboutComp; AboutPage {} }

    component NavItem: Controls.ItemDelegate {
        id: navDelegate
        property string iconName: ""
        Layout.fillWidth: true
        contentItem: RowLayout {
            spacing: Kirigami.Units.largeSpacing
            Kirigami.Icon {
                source: navDelegate.iconName
                implicitWidth: Kirigami.Units.iconSizes.smallMedium
                implicitHeight: Kirigami.Units.iconSizes.smallMedium
            }
            Controls.Label {
                text: navDelegate.text
                Layout.fillWidth: true
            }
            Kirigami.Icon {
                source: "go-next-symbolic"
                implicitWidth: Kirigami.Units.iconSizes.small
                implicitHeight: Kirigami.Units.iconSizes.small
            }
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.FormLayout {
            Layout.fillWidth: true

            Controls.ComboBox {
                id: langCombo
                Kirigami.FormData.label: i18n("Language")
                model: [i18n("System default"), "English", "Português (Brasil)"]
                currentIndex: Math.max(0, page.langCodes.indexOf(Config.language))
                onActivated: {
                    Config.language = page.langCodes[currentIndex]
                    applicationWindow().showPassiveNotification(
                        i18n("Restart the app to apply the language change."), "long")
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        NavItem {
            text: i18n("Dependencies")
            iconName: "emblem-system-symbolic"
            onClicked: applicationWindow().pageStack.push(depsComp)
        }
        NavItem {
            text: i18n("About")
            iconName: "help-about-symbolic"
            onClicked: applicationWindow().pageStack.push(aboutComp)
        }
    }
}
