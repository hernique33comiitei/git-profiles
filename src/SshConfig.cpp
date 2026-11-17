#include "SshConfig.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>

QString SshConfig::defaultPath()
{
    return QDir::homePath() + QStringLiteral("/.ssh/config");
}

bool SshConfig::splitKeyword(const QString &line, QString &keyword, QString &value)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))) {
        return false;
    }
    static const QRegularExpression sep(QStringLiteral("[\\s=]+"));
    const QRegularExpressionMatch m = sep.match(trimmed);
    if (!m.hasMatch()) {
        keyword = trimmed;
        value.clear();
        return true;
    }
    keyword = trimmed.left(m.capturedStart());
    value = trimmed.mid(m.capturedEnd()).trimmed();
    return true;
}

bool SshConfig::parseDirective(const QString &line, QString &key, QString &value, QStringList &flags)
{
    static const QRegularExpression re(QStringLiteral("^\\s*#@gp\\s+(.*)$"));
    const QRegularExpressionMatch m = re.match(line);
    if (!m.hasMatch()) {
        return false;
    }
    const QStringList tokens =
        m.captured(1).split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (tokens.isEmpty()) {
        return false;
    }
    const QString first = tokens.first();
    const int eq = first.indexOf(QLatin1Char('='));
    if (eq < 0) {
        key = first;
        value.clear();
    } else {
        key = first.left(eq);
        value = first.mid(eq + 1);
    }
    flags = tokens.mid(1);
    return true;
}

void SshConfig::parseBlockLine(const QString &line, SshProfile &p)
{
    QString dKey, dValue;
    QStringList dFlags;
    if (parseDirective(line, dKey, dValue, dFlags)) {
        if (dKey == QLatin1String("provider")) {
            p.provider = dValue;
        } else if (dKey == QLatin1String("apiurl")) {
            p.apiUrl = dValue;
        } else if (dKey == QLatin1String("ghconfig")) {
            p.ghConfig = dValue;
        } else if (dKey == QLatin1String("owner")) {
            p.owner = dValue;
        } else if (dKey == QLatin1String("token-ref")) {
            p.tokenRef = dValue;
        } else if (dKey == QLatin1String("dir")) {
            SshDir dir;
            dir.path = dValue;
            dir.children = dFlags.contains(QLatin1String("children"));
            p.dirs.append(dir);
        }
        return;
    }

    QString keyword, value;
    if (!splitKeyword(line, keyword, value)) {
        // blank or plain comment inside a block: preserve verbatim (trimmed)
        const QString t = line.trimmed();
        if (!t.isEmpty()) {
            p.extraOptions.append(t);
        }
        return;
    }

    const QString kw = keyword.toLower();
    if (kw == QLatin1String("hostname")) {
        p.hostName = value;
    } else if (kw == QLatin1String("user")) {
        p.user = value;
    } else if (kw == QLatin1String("identityfile")) {
        p.identityFile = value;
    } else if (kw == QLatin1String("port")) {
        p.port = value;
    } else {
        p.extraOptions.append(keyword + QLatin1Char(' ') + value);
    }
}

bool SshConfig::load(const QString &path)
{
    m_nodes.clear();
    m_error.clear();

    QFile f(path);
    if (!f.exists()) {
        return true; // empty config is a valid state
    }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_error = QStringLiteral("Cannot open %1: %2").arg(path, f.errorString());
        return false;
    }

    QStringList lines;
    {
        QTextStream in(&f);
        while (!in.atEnd()) {
            lines << in.readLine();
        }
    }

    const QRegularExpression wsSplit(QStringLiteral("\\s+"));
    auto isHeader = [](const QString &line, const char *kw) {
        QString k, v;
        return splitKeyword(line, k, v) && k.compare(QLatin1String(kw), Qt::CaseInsensitive) == 0;
    };

    Node rawNode;
    int i = 0;
    while (i < lines.size()) {
        const QString &line = lines.at(i);

        if (isHeader(line, "match")) {
            rawNode.raw.append(line); // Match blocks stay as passthrough
            ++i;
            continue;
        }
        if (isHeader(line, "host")) {
            if (!rawNode.raw.isEmpty()) {
                m_nodes.append(rawNode);
                rawNode = Node();
            }
            Node blk;
            blk.isBlock = true;
            blk.raw.append(line);
            QString k, v;
            splitKeyword(line, k, v);
            blk.profile.hostAliases = v.split(wsSplit, Qt::SkipEmptyParts);
            ++i;
            while (i < lines.size()) {
                const QString &l = lines.at(i);
                if (l.trimmed().isEmpty() || isHeader(l, "host") || isHeader(l, "match")) {
                    break;
                }
                blk.raw.append(l);
                parseBlockLine(l, blk.profile);
                ++i;
            }
            m_nodes.append(blk);
            continue;
        }

        rawNode.raw.append(line);
        ++i;
    }
    if (!rawNode.raw.isEmpty()) {
        m_nodes.append(rawNode);
    }

    return true;
}

QList<SshProfile> SshConfig::profiles() const
{
    QList<SshProfile> out;
    for (const Node &n : m_nodes) {
        if (n.isBlock) {
            out.append(n.profile);
        }
    }
    return out;
}

const SshProfile *SshConfig::find(const QString &host) const
{
    for (const Node &n : m_nodes) {
        if (n.isBlock && n.profile.host() == host) {
            return &n.profile;
        }
    }
    return nullptr;
}

void SshConfig::upsert(const SshProfile &p)
{
    for (Node &n : m_nodes) {
        if (n.isBlock && n.profile.host() == p.host()) {
            n.profile = p;
            n.dirty = true;
            return;
        }
    }
    // New profile: ensure a blank separator, then append a fresh block.
    if (!m_nodes.isEmpty()) {
        Node &last = m_nodes.last();
        if (last.raw.isEmpty() || !last.raw.last().trimmed().isEmpty()) {
            Node sep;
            sep.raw.append(QString());
            m_nodes.append(sep);
        }
    }
    Node blk;
    blk.isBlock = true;
    blk.profile = p;
    blk.dirty = true; // raw empty -> rendered on save
    m_nodes.append(blk);
}

void SshConfig::remove(const QString &host)
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes.at(i).isBlock && m_nodes.at(i).profile.host() == host) {
            m_nodes.removeAt(i);
            return;
        }
    }
}

QStringList SshConfig::renderBlock(const SshProfile &p)
{
    const QString indent = QStringLiteral("    ");
    QStringList out;
    out << QStringLiteral("Host ") + p.hostAliases.join(QLatin1Char(' '));
    if (!p.hostName.isEmpty()) {
        out << indent + QStringLiteral("HostName ") + p.hostName;
    }
    if (!p.user.isEmpty()) {
        out << indent + QStringLiteral("User ") + p.user;
    }
    if (!p.port.isEmpty()) {
        out << indent + QStringLiteral("Port ") + p.port;
    }
    if (!p.identityFile.isEmpty()) {
        out << indent + QStringLiteral("IdentityFile ") + p.identityFile;
    }
    for (const QString &extra : p.extraOptions) {
        out << indent + extra;
    }
    if (!p.provider.isEmpty()) {
        out << indent + QStringLiteral("#@gp provider=") + p.provider;
    }
    if (!p.apiUrl.isEmpty()) {
        out << indent + QStringLiteral("#@gp apiurl=") + p.apiUrl;
    }
    if (!p.owner.isEmpty()) {
        out << indent + QStringLiteral("#@gp owner=") + p.owner;
    }
    if (!p.ghConfig.isEmpty()) {
        out << indent + QStringLiteral("#@gp ghconfig=") + p.ghConfig;
    }
    if (!p.tokenRef.isEmpty()) {
        out << indent + QStringLiteral("#@gp token-ref=") + p.tokenRef;
    }
    for (const SshDir &d : p.dirs) {
        QString line = indent + QStringLiteral("#@gp dir=") + d.path;
        if (d.children) {
            line += QStringLiteral(" children");
        }
        out << line;
    }
    return out;
}

bool SshConfig::validate(const QString &path, QString *err)
{
    QProcess p;
    p.start(QStringLiteral("ssh"),
            {QStringLiteral("-F"), path, QStringLiteral("-G"), QStringLiteral("gitprofiles-probe")});
    if (!p.waitForStarted(1500)) {
        return true; // ssh unavailable: skip validation rather than block
    }
    p.waitForFinished(3000);
    if (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0) {
        return true;
    }
    if (err) {
        *err = QString::fromUtf8(p.readAllStandardError()).trimmed();
    }
    return false;
}

bool SshConfig::save(const QString &path, QString *backupPath)
{
    m_error.clear();

    QStringList outLines;
    for (const Node &n : m_nodes) {
        if (n.isBlock && (n.dirty || n.raw.isEmpty())) {
            outLines += renderBlock(n.profile);
        } else {
            outLines += n.raw;
        }
    }
    QString content = outLines.join(QLatin1Char('\n'));
    if (!content.endsWith(QLatin1Char('\n'))) {
        content += QLatin1Char('\n');
    }

    const QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());

    QString backup;
    if (QFile::exists(path)) {
        backup = path + QStringLiteral(".bak-")
            + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
        QFile::remove(backup);
        if (!QFile::copy(path, backup)) {
            m_error = QStringLiteral("Could not create backup %1").arg(backup);
            return false;
        }
        if (backupPath) {
            *backupPath = backup;
        }
    }

    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_error = QStringLiteral("Cannot write %1: %2").arg(path, out.errorString());
        return false;
    }
    out.write(content.toUtf8());
    if (!out.commit()) {
        m_error = QStringLiteral("Cannot save %1: %2").arg(path, out.errorString());
        return false;
    }
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    QString verr;
    if (!validate(path, &verr)) {
        if (!backup.isEmpty()) {
            QFile::remove(path);
            QFile::copy(backup, path);
            QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        }
        m_error = QStringLiteral("ssh rejected the new config, restored backup. %1").arg(verr);
        return false;
    }

    return true;
}
