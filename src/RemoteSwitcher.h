#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Rewrites the git remote of repositories to a profile's ssh Host alias.
// For normal repos it edits .git/config directly (fast, no process spawn) and
// runs repos in parallel; worktrees/submodules fall back to `git remote set-url`.
class RemoteSwitcher : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit RemoteSwitcher(QObject *parent = nullptr);

    // repos: list of { path, origin }. Computes the target URL (host -> alias)
    // for each and, unless dryRun, applies the change to every repo that needs
    // it (in parallel). Returns:
    //   { total, changed, failed, unchanged, noOrigin, dryRun,
    //     log: [ { path, status, ok, message, oldUrl, newUrl } ] }
    Q_INVOKABLE QVariantMap apply(const QString &alias, const QVariantList &repos, bool dryRun,
                                  const QString &remoteName = QStringLiteral("origin"));

    // Sets a remote's URL in one repo. Returns true on success.
    static bool setRemoteUrl(const QString &repoDir, const QString &remoteName,
                             const QString &newUrl, QString *err);
};
