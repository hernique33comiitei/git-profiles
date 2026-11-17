#pragma once

#include <QList>
#include <QString>
#include <QStringList>

// One directory attached to a profile, with the optional recursive flag
// ("children" = this dir + git repos one level below).
struct SshDir {
    QString path;
    bool children = false;
};

// One `Host` block of ~/.ssh/config, plus the extra metadata the app stores as
// `#@gp key=value` comment directives inside the block (ssh ignores them).
struct SshProfile {
    QStringList hostAliases; // every token on the `Host` line
    QString hostName;
    QString user;
    QString identityFile;
    QString port;

    // From `#@gp ...` directives:
    QString provider; // "", "github", "gitea", "gitlab"
    QString apiUrl; // provider API base, e.g. https://gitea.example.com
    QString ghConfig;
    QString owner;
    QString tokenRef;
    QList<SshDir> dirs;

    // Any other option lines inside the block (e.g. IdentitiesOnly), preserved
    // verbatim (trimmed) so a rewrite never drops the user's own settings.
    QStringList extraOptions;

    QString host() const
    {
        return hostAliases.isEmpty() ? QString() : hostAliases.first();
    }
};

// Parser + safe round-trip writer for ~/.ssh/config. The file is modelled as an
// ordered list of nodes (raw passthrough chunks and Host blocks); only blocks
// the app actually changes are re-rendered on save — everything else is kept
// byte-for-byte.
class SshConfig
{
public:
    static QString defaultPath();

    bool load(const QString &path = defaultPath());
    QList<SshProfile> profiles() const;
    QString error() const { return m_error; }

    const SshProfile *find(const QString &host) const;

    // In-memory mutations; call save() to persist.
    void upsert(const SshProfile &p); // match by primary host alias
    void remove(const QString &host);

    // Writes atomically with a timestamped backup, then validates the result
    // with `ssh -G`. On validation failure it restores the backup and fails.
    bool save(const QString &path = defaultPath(), QString *backupPath = nullptr);

private:
    struct Node {
        bool isBlock = false;
        QStringList raw; // original lines, verbatim
        SshProfile profile; // valid when isBlock
        bool dirty = false; // re-render on save when true (or raw empty)
    };

    static bool splitKeyword(const QString &line, QString &keyword, QString &value);
    static bool parseDirective(const QString &line, QString &key, QString &value, QStringList &flags);
    static void parseBlockLine(const QString &line, SshProfile &p);
    static QStringList renderBlock(const SshProfile &p);
    static bool validate(const QString &path, QString *err);

    QList<Node> m_nodes;
    QString m_error;
};
