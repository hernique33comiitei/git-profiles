#include "RemoteSwitcher.h"

#include "RepoScanner.h"

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QtConcurrent>

RemoteSwitcher::RemoteSwitcher(QObject *parent)
    : QObject(parent)
{
}

namespace {

// Rewrites `url = ...` under [remote "<name>"] in a .git/config, preserving the
// rest of the file. Returns false if the section or the url line is missing.
bool editGitConfigUrl(const QString &configPath, const QString &remoteName, const QString &newUrl,
                      QString *err)
{
    QFile f(configPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (err) {
            *err = QStringLiteral("cannot open %1").arg(configPath);
        }
        return false;
    }
    QStringList lines;
    {
        QTextStream in(&f);
        while (!in.atEnd()) {
            lines << in.readLine();
        }
    }
    f.close();

    const QString wanted = QStringLiteral("[remote \"%1\"]").arg(remoteName);
    int section = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines.at(i).trimmed() == wanted) {
            section = i;
            break;
        }
    }
    if (section < 0) {
        if (err) {
            *err = QStringLiteral("remote \"%1\" not found").arg(remoteName);
        }
        return false;
    }

    static const QRegularExpression urlRe(QStringLiteral("^(\\s*)url\\s*="));
    int urlLine = -1;
    QString indent = QStringLiteral("\t");
    for (int i = section + 1; i < lines.size(); ++i) {
        if (lines.at(i).trimmed().startsWith(QLatin1Char('['))) {
            break; // next section
        }
        const QRegularExpressionMatch m = urlRe.match(lines.at(i));
        if (m.hasMatch()) {
            urlLine = i;
            indent = m.captured(1).isEmpty() ? QStringLiteral("\t") : m.captured(1);
            break;
        }
    }
    if (urlLine < 0) {
        if (err) {
            *err = QStringLiteral("url not set for remote \"%1\"").arg(remoteName);
        }
        return false;
    }

    lines[urlLine] = indent + QStringLiteral("url = ") + newUrl;

    QSaveFile out(configPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (err) {
            *err = out.errorString();
        }
        return false;
    }
    out.write((lines.join(QLatin1Char('\n')) + QLatin1Char('\n')).toUtf8());
    if (!out.commit()) {
        if (err) {
            *err = out.errorString();
        }
        return false;
    }
    return true;
}

bool gitSetUrl(const QString &repoDir, const QString &remoteName, const QString &newUrl, QString *err)
{
    QProcess p;
    p.start(QStringLiteral("git"),
            {QStringLiteral("-C"), repoDir, QStringLiteral("remote"), QStringLiteral("set-url"),
             remoteName, newUrl});
    if (!p.waitForStarted(1500)) {
        if (err) {
            *err = QStringLiteral("git not available");
        }
        return false;
    }
    p.waitForFinished(5000);
    if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) {
        return true;
    }
    if (err) {
        *err = QString::fromUtf8(p.readAllStandardError()).trimmed();
    }
    return false;
}

} // namespace

bool RemoteSwitcher::setRemoteUrl(const QString &repoDir, const QString &remoteName,
                                  const QString &newUrl, QString *err)
{
    const QString gitPath = repoDir + QStringLiteral("/.git");
    if (QFileInfo(gitPath).isDir()) {
        QString directErr;
        if (editGitConfigUrl(gitPath + QStringLiteral("/config"), remoteName, newUrl, &directErr)) {
            return true;
        }
        // Fall back to git for anything the direct editor cannot handle.
        return gitSetUrl(repoDir, remoteName, newUrl, err);
    }
    // Worktree / submodule: .git is a file, let git resolve the real config.
    return gitSetUrl(repoDir, remoteName, newUrl, err);
}

QVariantMap RemoteSwitcher::apply(const QString &alias, const QVariantList &repos, bool dryRun,
                                  const QString &remoteName)
{
    const QString remote = remoteName.isEmpty() ? QStringLiteral("origin") : remoteName;

    struct Item {
        QString path;
        QString oldUrl;
        QString newUrl;
        QString status; // "change" | "unchanged" | "no-origin"
    };

    QList<Item> items;
    items.reserve(repos.size());
    int unchanged = 0;
    int noOrigin = 0;
    for (const QVariant &v : repos) {
        const QVariantMap m = v.toMap();
        Item it;
        it.path = m.value(QStringLiteral("path")).toString();
        it.oldUrl = m.value(QStringLiteral("origin")).toString();
        it.newUrl = RepoScanner::rewriteUrl(it.oldUrl, alias);
        if (it.oldUrl.isEmpty()) {
            it.status = QStringLiteral("no-origin");
            ++noOrigin;
        } else if (it.oldUrl == it.newUrl) {
            it.status = QStringLiteral("unchanged");
            ++unchanged;
        } else {
            it.status = QStringLiteral("change");
        }
        items.append(it);
    }

    QList<Item> toChange;
    for (const Item &it : items) {
        if (it.status == QLatin1String("change")) {
            toChange.append(it);
        }
    }

    QHash<QString, QString> applyError; // path -> error ("" means ok)
    int changed = 0;
    int failed = 0;

    if (!dryRun && !toChange.isEmpty()) {
        const QString remoteName = remote;
        const auto results = QtConcurrent::blockingMapped(
            toChange, [remoteName](const Item &it) -> QPair<QString, QString> {
                QString err;
                const bool ok = setRemoteUrl(it.path, remoteName, it.newUrl, &err);
                return {it.path, ok ? QString() : (err.isEmpty() ? QStringLiteral("failed") : err)};
            });
        for (const auto &r : results) {
            applyError.insert(r.first, r.second);
            if (r.second.isEmpty()) {
                ++changed;
            } else {
                ++failed;
            }
        }
    }

    QVariantList log;
    for (const Item &it : items) {
        QVariantMap entry;
        entry[QStringLiteral("path")] = it.path;
        entry[QStringLiteral("status")] = it.status;
        entry[QStringLiteral("oldUrl")] = it.oldUrl;
        entry[QStringLiteral("newUrl")] = it.newUrl;
        if (it.status == QLatin1String("change") && !dryRun) {
            const QString err = applyError.value(it.path);
            entry[QStringLiteral("ok")] = err.isEmpty();
            entry[QStringLiteral("message")] = err;
        } else {
            entry[QStringLiteral("ok")] = true;
            entry[QStringLiteral("message")] = QString();
        }
        log.append(entry);
    }

    QVariantMap out;
    out[QStringLiteral("total")] = items.size();
    out[QStringLiteral("toChange")] = toChange.size();
    out[QStringLiteral("changed")] = changed;
    out[QStringLiteral("failed")] = failed;
    out[QStringLiteral("unchanged")] = unchanged;
    out[QStringLiteral("noOrigin")] = noOrigin;
    out[QStringLiteral("dryRun")] = dryRun;
    out[QStringLiteral("log")] = log;
    return out;
}
