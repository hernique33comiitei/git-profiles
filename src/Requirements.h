#pragma once

#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantMap>

// Preflight for external tools. The app installs nothing; it only checks and
// reports. `git` is a hard requirement (the UI blocks without it). The provider
// CLIs (gh / gt / glab) are checked lazily, per provider, against pinned
// minimum versions frozen on 2026-11-16 for app v0.0.1.
class Requirements : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool gitInstalled READ gitInstalled NOTIFY changed)
    Q_PROPERTY(QString gitVersion READ gitVersion NOTIFY changed)
    Q_PROPERTY(QString gitUrl READ gitUrl CONSTANT)

public:
    explicit Requirements(QObject *parent = nullptr);

    bool gitInstalled();
    QString gitVersion();
    QString gitUrl() const;

    // id: "github" | "gitea" | "gitlab".
    // Returns { valid, id, cli, installed, version, required, ok, url }.
    Q_INVOKABLE QVariantMap provider(const QString &id);

    // Drops cached detections and re-evaluates on next read.
    Q_INVOKABLE void recheck();

Q_SIGNALS:
    void changed();

private:
    struct Tool {
        QString cli;
        QString version;
        bool installed = false;
    };

    Tool detect(const QString &cli) const; // runs `<cli> --version`
    static QString parseVersion(const QString &output);
    static int compareVersions(const QString &a, const QString &b);

    bool m_gitDetected = false;
    Tool m_git;
    QHash<QString, Tool> m_cache;
};
