#include "Secrets.h"

#include <QProcess>
#include <QStandardPaths>

Secrets::Secrets(QObject *parent)
    : QObject(parent)
{
}

QString Secrets::secretToolPath()
{
    return QStandardPaths::findExecutable(QStringLiteral("secret-tool"));
}

bool Secrets::available() const
{
    return !secretToolPath().isEmpty();
}

QString Secrets::getToken(const QString &profile)
{
    const QString tool = secretToolPath();
    if (tool.isEmpty() || profile.isEmpty()) {
        return {};
    }
    QProcess p;
    p.start(tool,
            {QStringLiteral("lookup"), QStringLiteral("service"), QStringLiteral("gitprofiles"),
             QStringLiteral("profile"), profile});
    if (!p.waitForStarted(1500)) {
        return {};
    }
    p.waitForFinished(3000);
    QString out = QString::fromUtf8(p.readAllStandardOutput());
    while (out.endsWith(QLatin1Char('\n')) || out.endsWith(QLatin1Char('\r'))) {
        out.chop(1);
    }
    return out;
}

bool Secrets::hasToken(const QString &profile) const
{
    return !getToken(profile).isEmpty();
}

QString Secrets::setToken(const QString &profile, const QString &token)
{
    const QString tool = secretToolPath();
    if (tool.isEmpty()) {
        return QStringLiteral("secret-tool is not installed (needed to store tokens).");
    }
    if (profile.isEmpty()) {
        return QStringLiteral("Missing profile name.");
    }
    if (token.isEmpty()) {
        return QStringLiteral("The token is empty.");
    }
    QProcess p;
    p.start(tool,
            {QStringLiteral("store"), QStringLiteral("--label=git-profiles: ") + profile,
             QStringLiteral("service"), QStringLiteral("gitprofiles"), QStringLiteral("profile"),
             profile});
    if (!p.waitForStarted(1500)) {
        return QStringLiteral("Could not run secret-tool.");
    }
    p.write(token.toUtf8()); // secret read from stdin (no trailing newline)
    p.closeWriteChannel();
    p.waitForFinished(5000);
    if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) {
        return {};
    }
    return QString::fromUtf8(p.readAllStandardError()).trimmed();
}

QString Secrets::clearToken(const QString &profile)
{
    const QString tool = secretToolPath();
    if (tool.isEmpty()) {
        return QStringLiteral("secret-tool is not installed.");
    }
    QProcess p;
    p.start(tool,
            {QStringLiteral("clear"), QStringLiteral("service"), QStringLiteral("gitprofiles"),
             QStringLiteral("profile"), profile});
    if (!p.waitForStarted(1500)) {
        return QStringLiteral("Could not run secret-tool.");
    }
    p.waitForFinished(5000);
    if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) {
        return {};
    }
    return QString::fromUtf8(p.readAllStandardError()).trimmed();
}
