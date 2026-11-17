#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantMap>

// Configures the provider CLI (gh / gt / glab) per directory. It generates a
// direnv `.envrc` for each attached directory and, given a token, performs a
// non-interactive login into the right place (gh config dir, or the keyring that
// the .envrc reads back). The app installs nothing; it only drives existing CLIs.
class ProviderCli : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ProviderCli(QObject *parent = nullptr);

    // { direnvInstalled, direnvHook, direnvUrl, secretTool }
    Q_INVOKABLE QVariantMap environment() const;

    Q_INVOKABLE QString defaultGhConfigDir(const QString &host) const;
    Q_INVOKABLE QString homeDir() const;

    // The .envrc body for a profile (fields map), for preview and writing.
    Q_INVOKABLE QString envrc(const QVariantMap &profile) const;

    // Upserts the managed block into <dir>/.envrc and runs `direnv allow` when
    // direnv is present. Returns "" on success or an error message.
    Q_INVOKABLE QString writeEnvrc(const QString &dir, const QVariantMap &profile);

    // "absent" | "outdated" | "ok" — state of the managed block in <dir>/.envrc.
    Q_INVOKABLE QString envrcStatus(const QString &dir, const QVariantMap &profile) const;

    // Stores the token in the keyring and, when the provider CLI is installed,
    // performs a non-interactive token login. Returns "" on success or an error.
    Q_INVOKABLE QString setupCredential(const QVariantMap &profile, const QString &token);

private:
    static QString expandTilde(const QString &p);
    static QString stripScheme(const QString &url);
};
