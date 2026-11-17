#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>

// Finds git repositories under an attached directory and helps preview remote
// changes. Exposed to QML as a stateless singleton.
class RepoScanner : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit RepoScanner(QObject *parent = nullptr);

    // Expands a directory into git repos: the directory itself (if a repo) plus,
    // when `children` is set, every immediate subdirectory that is a repo.
    // Returns a list of { path, origin } maps.
    Q_INVOKABLE QVariantList scan(const QString &dir, bool children) const;

    // Lightweight count of git repos under `dir` (+ 1 level if children),
    // without reading remotes — for the directory list.
    Q_INVOKABLE int countRepos(const QString &dir, bool children) const;

    // Rewrites the host of an SSH remote URL to the given ssh Host alias,
    // preserving user and path. Non-SSH URLs are returned unchanged.
    Q_INVOKABLE QString rewrite(const QString &url, const QString &alias) const;

    static QString expandTilde(const QString &path);
    static bool isGitRepo(const QString &dir);
    static QString readOrigin(const QString &dir);
    static QString rewriteUrl(const QString &url, const QString &alias);
};
