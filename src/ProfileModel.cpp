#include "ProfileModel.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QVariantList>
#include <QVariantMap>

namespace {
QString expandTilde(const QString &p)
{
    if (p == QLatin1String("~")) {
        return QDir::homePath();
    }
    if (p.startsWith(QLatin1String("~/"))) {
        return QDir::homePath() + p.mid(1);
    }
    return p;
}

// Store paths in ~-form when they live under $HOME, matching the user's style.
QString tildify(const QString &path)
{
    const QString p = path.trimmed();
    const QString home = QDir::homePath();
    if (p == home) {
        return QStringLiteral("~");
    }
    if (p.startsWith(home + QLatin1Char('/'))) {
        return QStringLiteral("~") + p.mid(home.size());
    }
    return p;
}
} // namespace

ProfileModel::ProfileModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_path(SshConfig::defaultPath())
{
    reload();
}

void ProfileModel::setPath(const QString &path)
{
    if (m_path == path) {
        return;
    }
    m_path = path;
    Q_EMIT pathChanged();
    reload();
}

void ProfileModel::reload()
{
    SshConfig config;
    const bool ok = config.load(m_path);

    beginResetModel();
    m_profiles = config.profiles();
    endResetModel();

    const QString newError = ok ? QString() : config.error();
    if (newError != m_error) {
        m_error = newError;
        Q_EMIT errorChanged();
    }
    Q_EMIT countChanged();
}

int ProfileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_profiles.size());
}

QVariant ProfileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_profiles.size()) {
        return {};
    }
    const SshProfile &p = m_profiles.at(index.row());

    switch (role) {
    case HostRole:
        return p.host();
    case HostNameRole:
        return p.hostName;
    case UserRole:
        return p.user;
    case IdentityFileRole:
        return p.identityFile;
    case PortRole:
        return p.port;
    case ProviderRole:
        return p.provider;
    case OwnerRole:
        return p.owner;
    case DirCountRole:
        return p.dirs.size();
    case DirsRole: {
        QVariantList list;
        for (const SshDir &d : p.dirs) {
            QVariantMap m;
            m[QStringLiteral("path")] = d.path;
            m[QStringLiteral("children")] = d.children;
            list.append(m);
        }
        return list;
    }
    case SubtitleRole: {
        QStringList bits;
        if (!p.hostName.isEmpty()) {
            bits << p.hostName + (p.port.isEmpty() ? QString() : QLatin1Char(':') + p.port);
        }
        if (!p.user.isEmpty()) {
            bits << p.user + QStringLiteral("@");
        }
        return bits.join(QStringLiteral("  ·  "));
    }
    case IdentityExistsRole:
        return p.identityFile.isEmpty() || QFileInfo::exists(expandTilde(p.identityFile));
    default:
        return {};
    }
}

QVariantMap ProfileModel::profileAt(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= m_profiles.size()) {
        return m;
    }
    const SshProfile &p = m_profiles.at(row);
    m[QStringLiteral("host")] = p.hostAliases.join(QLatin1Char(' '));
    m[QStringLiteral("hostName")] = p.hostName;
    m[QStringLiteral("user")] = p.user;
    m[QStringLiteral("port")] = p.port;
    m[QStringLiteral("identityFile")] = p.identityFile;
    m[QStringLiteral("provider")] = p.provider;
    m[QStringLiteral("apiUrl")] = p.apiUrl;
    m[QStringLiteral("owner")] = p.owner;
    m[QStringLiteral("ghConfig")] = p.ghConfig;
    return m;
}

QVariantMap ProfileModel::profileByHost(const QString &host) const
{
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles.at(i).host() == host) {
            return profileAt(i);
        }
    }
    return {};
}

QString ProfileModel::saveProfile(const QString &originalHost, const QVariantMap &fields)
{
    SshConfig cfg;
    if (!cfg.load(m_path)) {
        return cfg.error().isEmpty() ? QStringLiteral("Could not read the config file.") : cfg.error();
    }

    const QString host = fields.value(QStringLiteral("host")).toString().trimmed();
    if (host.isEmpty()) {
        return QStringLiteral("The Host alias is required.");
    }

    SshProfile p;
    if (!originalHost.isEmpty()) {
        if (const SshProfile *existing = cfg.find(originalHost)) {
            p = *existing; // preserve dirs and extra options
        }
    } else if (cfg.find(host) != nullptr) {
        return QStringLiteral("A profile named \"%1\" already exists.").arg(host);
    }

    p.hostAliases = host.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    p.hostName = fields.value(QStringLiteral("hostName")).toString().trimmed();
    p.user = fields.value(QStringLiteral("user")).toString().trimmed();
    p.port = fields.value(QStringLiteral("port")).toString().trimmed();
    p.identityFile = fields.value(QStringLiteral("identityFile")).toString().trimmed();
    p.provider = fields.value(QStringLiteral("provider")).toString().trimmed();
    p.apiUrl = fields.value(QStringLiteral("apiUrl")).toString().trimmed();
    p.owner = fields.value(QStringLiteral("owner")).toString().trimmed();
    p.ghConfig = fields.value(QStringLiteral("ghConfig")).toString().trimmed();

    if (!originalHost.isEmpty() && p.host() != originalHost) {
        cfg.remove(originalHost);
    }
    cfg.upsert(p);

    const bool ok = cfg.save(m_path);
    const QString err = ok ? QString() : cfg.error();
    reload();
    return ok ? QString() : (err.isEmpty() ? QStringLiteral("Could not save the config file.") : err);
}

QString ProfileModel::removeProfile(const QString &host)
{
    SshConfig cfg;
    if (!cfg.load(m_path)) {
        return cfg.error().isEmpty() ? QStringLiteral("Could not read the config file.") : cfg.error();
    }
    cfg.remove(host);
    const bool ok = cfg.save(m_path);
    const QString err = ok ? QString() : cfg.error();
    reload();
    return ok ? QString() : (err.isEmpty() ? QStringLiteral("Could not save the config file.") : err);
}

QVariantList ProfileModel::directoriesOf(const QString &host) const
{
    QVariantList out;
    for (const SshProfile &p : m_profiles) {
        if (p.host() == host) {
            for (const SshDir &d : p.dirs) {
                QVariantMap m;
                m[QStringLiteral("path")] = d.path;
                m[QStringLiteral("children")] = d.children;
                out.append(m);
            }
            break;
        }
    }
    return out;
}

QString ProfileModel::attachDirectory(const QString &host, const QString &path, bool children)
{
    const QString p = tildify(path);
    if (p.isEmpty()) {
        return QStringLiteral("A directory path is required.");
    }
    SshConfig cfg;
    if (!cfg.load(m_path)) {
        return cfg.error().isEmpty() ? QStringLiteral("Could not read the config file.") : cfg.error();
    }
    const SshProfile *existing = cfg.find(host);
    if (!existing) {
        return QStringLiteral("Profile \"%1\" was not found.").arg(host);
    }
    SshProfile prof = *existing;
    for (const SshDir &d : prof.dirs) {
        if (d.path == p) {
            return QStringLiteral("\"%1\" is already attached to this profile.").arg(p);
        }
    }
    prof.dirs.append(SshDir{p, children});
    cfg.upsert(prof);
    const bool ok = cfg.save(m_path);
    const QString err = ok ? QString() : cfg.error();
    reload();
    return ok ? QString() : (err.isEmpty() ? QStringLiteral("Could not save the config file.") : err);
}

QString ProfileModel::detachDirectory(const QString &host, const QString &path)
{
    SshConfig cfg;
    if (!cfg.load(m_path)) {
        return cfg.error().isEmpty() ? QStringLiteral("Could not read the config file.") : cfg.error();
    }
    const SshProfile *existing = cfg.find(host);
    if (!existing) {
        return QStringLiteral("Profile \"%1\" was not found.").arg(host);
    }
    SshProfile prof = *existing;
    for (int i = 0; i < prof.dirs.size(); ++i) {
        if (prof.dirs.at(i).path == path) {
            prof.dirs.removeAt(i);
            break;
        }
    }
    cfg.upsert(prof);
    const bool ok = cfg.save(m_path);
    const QString err = ok ? QString() : cfg.error();
    reload();
    return ok ? QString() : (err.isEmpty() ? QStringLiteral("Could not save the config file.") : err);
}

QString ProfileModel::setDirChildren(const QString &host, const QString &path, bool children)
{
    SshConfig cfg;
    if (!cfg.load(m_path)) {
        return cfg.error().isEmpty() ? QStringLiteral("Could not read the config file.") : cfg.error();
    }
    const SshProfile *existing = cfg.find(host);
    if (!existing) {
        return QStringLiteral("Profile \"%1\" was not found.").arg(host);
    }
    SshProfile prof = *existing;
    for (SshDir &d : prof.dirs) {
        if (d.path == path) {
            d.children = children;
            break;
        }
    }
    cfg.upsert(prof);
    const bool ok = cfg.save(m_path);
    const QString err = ok ? QString() : cfg.error();
    reload();
    return ok ? QString() : (err.isEmpty() ? QStringLiteral("Could not save the config file.") : err);
}

QHash<int, QByteArray> ProfileModel::roleNames() const
{
    return {
        {HostRole, "host"},
        {HostNameRole, "hostName"},
        {UserRole, "user"},
        {IdentityFileRole, "identityFile"},
        {PortRole, "port"},
        {ProviderRole, "provider"},
        {OwnerRole, "owner"},
        {DirCountRole, "dirCount"},
        {DirsRole, "dirs"},
        {SubtitleRole, "subtitle"},
        {IdentityExistsRole, "identityExists"},
    };
}

QStringList ProfileModel::providersInUse() const
{
    QStringList out;
    for (const SshProfile &p : m_profiles) {
        if (!p.provider.isEmpty() && !out.contains(p.provider)) {
            out.append(p.provider);
        }
    }
    return out;
}

bool ProfileModel::hostTaken(const QString &host, const QString &exceptHost) const
{
    for (const SshProfile &p : m_profiles) {
        if (p.host() == host && p.host() != exceptHost) {
            return true;
        }
    }
    return false;
}
