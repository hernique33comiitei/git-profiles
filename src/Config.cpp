#include "Config.h"

Config::Config(QObject *parent)
    : QObject(parent)
{
}

QString Config::language() const
{
    return m_settings.value(QStringLiteral("language")).toString();
}

void Config::setLanguage(const QString &lang)
{
    if (language() == lang) {
        return;
    }
    m_settings.setValue(QStringLiteral("language"), lang);
    m_settings.sync();
    Q_EMIT languageChanged();
}

QString Config::version() const
{
    return QStringLiteral(GITPROFILES_VERSION);
}

QString Config::storedLanguage()
{
    QSettings settings;
    return settings.value(QStringLiteral("language")).toString();
}
