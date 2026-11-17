#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include "SshConfig.h"

// Exposes the profiles parsed from ~/.ssh/config to QML as a list model.
class ProfileModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    enum Roles {
        HostRole = Qt::UserRole + 1,
        HostNameRole,
        UserRole,
        IdentityFileRole,
        PortRole,
        ProviderRole,
        OwnerRole,
        DirCountRole,
        DirsRole,
        SubtitleRole,
        IdentityExistsRole,
    };
    Q_ENUM(Roles)

    explicit ProfileModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString path() const { return m_path; }
    void setPath(const QString &path);
    QString error() const { return m_error; }

    Q_INVOKABLE void reload();

    // Returns the editable fields of the row as
    // { host, hostName, user, port, identityFile, provider, apiUrl, owner, ghConfig }.
    Q_INVOKABLE QVariantMap profileAt(int row) const;
    Q_INVOKABLE QVariantMap profileByHost(const QString &host) const;

    // Distinct non-empty providers used across all profiles (for Settings).
    Q_INVOKABLE QStringList providersInUse() const;
    // True if `host` already names a profile other than `exceptHost` (validation).
    Q_INVOKABLE bool hostTaken(const QString &host, const QString &exceptHost) const;

    // Create or update a profile, writing ~/.ssh/config safely. Pass an empty
    // originalHost to create. Returns "" on success, or an error message.
    Q_INVOKABLE QString saveProfile(const QString &originalHost, const QVariantMap &fields);

    // Deletes a profile by host alias. Returns "" on success, or an error.
    Q_INVOKABLE QString removeProfile(const QString &host);

    // Directories attached to a profile: [{ path, children }].
    Q_INVOKABLE QVariantList directoriesOf(const QString &host) const;
    Q_INVOKABLE QString attachDirectory(const QString &host, const QString &path, bool children);
    Q_INVOKABLE QString detachDirectory(const QString &host, const QString &path);
    Q_INVOKABLE QString setDirChildren(const QString &host, const QString &path, bool children);

Q_SIGNALS:
    void pathChanged();
    void countChanged();
    void errorChanged();

private:
    QString m_path;
    QString m_error;
    QList<SshProfile> m_profiles;
};
