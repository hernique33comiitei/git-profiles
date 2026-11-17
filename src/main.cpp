#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>

#include <KAboutData>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include "Config.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("gitprofiles"));
    QCoreApplication::setOrganizationName(QStringLiteral("gitprofiles"));
    QCoreApplication::setApplicationName(QStringLiteral("gitprofiles"));

    // Apply the user's language override (if any) before any i18n() call.
    const QString lang = Config::storedLanguage();
    if (!lang.isEmpty()) {
        KLocalizedString::setLanguages({lang});
    }

    KAboutData aboutData(QStringLiteral("gitprofiles"),
                         i18n("Git Profiles"),
                         QStringLiteral(GITPROFILES_VERSION),
                         i18n("Organize your git repositories by SSH profile"),
                         KAboutLicense::MIT,
                         i18n("© 2026 Henrique Oliveira"));
    aboutData.setDesktopFileName(QStringLiteral("io.github.hernique33comiitei.GitProfiles"));
    aboutData.setHomepage(QStringLiteral("https://github.com/hernique33comiitei"));
    aboutData.addAuthor(QStringLiteral("Henrique Oliveira"), i18n("Author"), QString(),
                        QStringLiteral("https://github.com/hernique33comiitei"));
    KAboutData::setApplicationData(aboutData);

    // Use the bundled icon directly so it never resolves to a stale themed copy.
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/data/icons/gitprofiles.svg")));

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.loadFromModule("org.gitprofiles", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
