#include "Requirements.h"

#include <QProcess>
#include <QRegularExpression>
#include <algorithm>

namespace {
struct Pin {
    const char *provider;
    const char *cli;
    const char *minVersion;
    const char *url;
};

// Pins frozen on 2026-11-16 for app v0.0.1. Bump together with the app version.
const Pin PINS[] = {
    {"github", "gh", "2.104.0", "https://github.com/cli/cli/releases"},
    {"gitea", "gt", "1.2.0", "https://gitea.com/gitea/cli/releases"},
    {"gitlab", "glab", "1.130.0", "https://gitlab.com/gitlab-org/cli/-/releases"},
};
} // namespace

Requirements::Requirements(QObject *parent)
    : QObject(parent)
{
}

QString Requirements::gitUrl() const
{
    return QStringLiteral("https://git-scm.com/downloads");
}

void Requirements::recheck()
{
    m_gitDetected = false;
    m_cache.clear();
    Q_EMIT changed();
}

Requirements::Tool Requirements::detect(const QString &cli) const
{
    Tool t;
    t.cli = cli;

    QProcess p;
    p.start(cli, {QStringLiteral("--version")});
    if (!p.waitForStarted(1500)) {
        return t; // not found / not executable
    }
    p.waitForFinished(3000);

    const QString out =
        QString::fromUtf8(p.readAllStandardOutput()) + QString::fromUtf8(p.readAllStandardError());
    t.version = parseVersion(out);
    t.installed = !t.version.isEmpty() || p.exitStatus() == QProcess::NormalExit;
    return t;
}

QString Requirements::parseVersion(const QString &output)
{
    static const QRegularExpression re(QStringLiteral("([0-9]+\\.[0-9]+\\.[0-9]+)"));
    const QRegularExpressionMatch m = re.match(output);
    return m.hasMatch() ? m.captured(1) : QString();
}

int Requirements::compareVersions(const QString &a, const QString &b)
{
    const QStringList pa = a.split(QLatin1Char('.'));
    const QStringList pb = b.split(QLatin1Char('.'));
    const int n = std::max(pa.size(), pb.size());
    for (int i = 0; i < n; ++i) {
        const int na = i < pa.size() ? pa.at(i).toInt() : 0;
        const int nb = i < pb.size() ? pb.at(i).toInt() : 0;
        if (na != nb) {
            return na < nb ? -1 : 1;
        }
    }
    return 0;
}

bool Requirements::gitInstalled()
{
    if (!m_gitDetected) {
        m_git = detect(QStringLiteral("git"));
        m_gitDetected = true;
    }
    return m_git.installed;
}

QString Requirements::gitVersion()
{
    gitInstalled();
    return m_git.version;
}

QVariantMap Requirements::provider(const QString &id)
{
    QVariantMap r;

    const Pin *pin = nullptr;
    for (const auto &p : PINS) {
        if (id == QLatin1String(p.provider)) {
            pin = &p;
            break;
        }
    }
    if (!pin) {
        r[QStringLiteral("valid")] = false;
        return r;
    }

    const QString cli = QString::fromLatin1(pin->cli);
    Tool t;
    if (m_cache.contains(cli)) {
        t = m_cache.value(cli);
    } else {
        t = detect(cli);
        m_cache.insert(cli, t);
    }

    const QString required = QString::fromLatin1(pin->minVersion);
    const bool versionOk = !t.version.isEmpty() && compareVersions(t.version, required) >= 0;

    r[QStringLiteral("valid")] = true;
    r[QStringLiteral("id")] = id;
    r[QStringLiteral("cli")] = cli;
    r[QStringLiteral("installed")] = t.installed;
    r[QStringLiteral("version")] = t.version;
    r[QStringLiteral("required")] = required;
    r[QStringLiteral("ok")] = t.installed && versionOk;
    r[QStringLiteral("url")] = QString::fromLatin1(pin->url);
    return r;
}
