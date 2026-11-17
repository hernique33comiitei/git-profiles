#include "RepoScanner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>
#include <QVariantMap>

RepoScanner::RepoScanner(QObject *parent)
    : QObject(parent)
{
}

QString RepoScanner::expandTilde(const QString &path)
{
    if (path == QLatin1String("~")) {
        return QDir::homePath();
    }
    if (path.startsWith(QLatin1String("~/"))) {
        return QDir::homePath() + path.mid(1);
    }
    return path;
}

bool RepoScanner::isGitRepo(const QString &dir)
{
    // A non-bare repo has a `.git` entry (directory for normal repos, a file for
    // worktrees and submodules). Bare repos have no `.git` and are skipped.
    return QFileInfo::exists(dir + QStringLiteral("/.git"));
}

static QString parseOriginFromConfig(const QString &configPath)
{
    QFile f(configPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QTextStream in(&f);
    bool inOrigin = false;
    static const QRegularExpression sectionRe(QStringLiteral("^\\s*\\[(.+)\\]\\s*$"));
    static const QRegularExpression urlRe(QStringLiteral("^\\s*url\\s*=\\s*(.+)$"));
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const QRegularExpressionMatch sm = sectionRe.match(line);
        if (sm.hasMatch()) {
            const QString section = sm.captured(1).trimmed();
            inOrigin = (section == QLatin1String("remote \"origin\""));
            continue;
        }
        if (inOrigin) {
            const QRegularExpressionMatch um = urlRe.match(line);
            if (um.hasMatch()) {
                return um.captured(1).trimmed();
            }
        }
    }
    return {};
}

QString RepoScanner::readOrigin(const QString &dir)
{
    const QString gitPath = dir + QStringLiteral("/.git");
    const QFileInfo gi(gitPath);
    if (gi.isDir()) {
        return parseOriginFromConfig(gitPath + QStringLiteral("/config"));
    }
    // Worktree/submodule (.git is a file): ask git directly.
    QProcess p;
    p.start(QStringLiteral("git"),
            {QStringLiteral("-C"), dir, QStringLiteral("config"), QStringLiteral("--get"),
             QStringLiteral("remote.origin.url")});
    if (!p.waitForStarted(1500)) {
        return {};
    }
    p.waitForFinished(3000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

QVariantList RepoScanner::scan(const QString &dir, bool children) const
{
    QVariantList out;
    const QString base = expandTilde(dir);

    auto add = [&out](const QString &repoPath) {
        QVariantMap m;
        m[QStringLiteral("path")] = repoPath;
        m[QStringLiteral("origin")] = readOrigin(repoPath);
        out.append(m);
    };

    if (isGitRepo(base)) {
        add(base);
    }
    if (children) {
        QDir d(base);
        const QStringList subs =
            d.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString &sub : subs) {
            const QString subPath = d.absoluteFilePath(sub);
            if (isGitRepo(subPath)) {
                add(subPath);
            }
        }
    }
    return out;
}

int RepoScanner::countRepos(const QString &dir, bool children) const
{
    const QString base = expandTilde(dir);
    int n = 0;
    if (isGitRepo(base)) {
        ++n;
    }
    if (children) {
        const QStringList subs = QDir(base).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString &sub : subs) {
            if (isGitRepo(QDir(base).absoluteFilePath(sub))) {
                ++n;
            }
        }
    }
    return n;
}

QString RepoScanner::rewrite(const QString &url, const QString &alias) const
{
    return rewriteUrl(url, alias);
}

QString RepoScanner::rewriteUrl(const QString &url, const QString &alias)
{
    if (alias.isEmpty() || url.isEmpty()) {
        return url;
    }

    // ssh://[user@]host[:port]/path
    static const QRegularExpression sshRe(
        QStringLiteral("^ssh://(?:([^@/]+)@)?([^:/]+)(?::\\d+)?/(.+)$"));
    const QRegularExpressionMatch sm = sshRe.match(url);
    if (sm.hasMatch()) {
        const QString user = sm.captured(1);
        const QString path = sm.captured(3);
        return QStringLiteral("ssh://") + (user.isEmpty() ? QString() : user + QLatin1Char('@'))
            + alias + QLatin1Char('/') + path;
    }

    // scp-like: [user@]host:path  (reject anything with "://")
    if (!url.contains(QLatin1String("://"))) {
        static const QRegularExpression scpRe(
            QStringLiteral("^(?:([^@]+)@)?([^:/]+):(.+)$"));
        const QRegularExpressionMatch cm = scpRe.match(url);
        if (cm.hasMatch()) {
            const QString user = cm.captured(1);
            const QString path = cm.captured(3);
            return (user.isEmpty() ? QString() : user + QLatin1Char('@')) + alias
                + QLatin1Char(':') + path;
        }
    }

    return url; // https or unrecognized: leave as-is
}
