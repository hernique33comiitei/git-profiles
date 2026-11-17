#include "SshTester.h"

#include <QDir>

SshTester::SshTester(QObject *parent)
    : QObject(parent)
{
}

void SshTester::setRunning(bool r)
{
    if (m_running == r) {
        return;
    }
    m_running = r;
    Q_EMIT runningChanged();
}

void SshTester::test(const QString &hostName, const QString &user,
                     const QString &port, const QString &identityFile)
{
    const QString host = hostName.trimmed();
    if (m_running || host.isEmpty()) {
        return;
    }

    m_proc = new QProcess(this);
    setRunning(true);

    connect(m_proc, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(m_proc->readAllStandardOutput())
            + QString::fromUtf8(m_proc->readAllStandardError());
        const QString lower = out.toLower();

        // First non-empty line, as a short summary of the server's reply.
        QString summary;
        const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            summary = lines.first().trimmed();
        }

        QString level;
        if (lower.contains(QLatin1String("success")) || lower.contains(QLatin1String("authenticated"))
            || lower.contains(QLatin1String("welcome")) || lower.contains(QLatin1String("hi "))) {
            level = QStringLiteral("success");
        } else if (lower.contains(QLatin1String("could not resolve"))
                   || lower.contains(QLatin1String("timed out"))
                   || lower.contains(QLatin1String("connection refused"))
                   || lower.contains(QLatin1String("no route to host"))) {
            level = QStringLiteral("unreachable");
        } else if (lower.contains(QLatin1String("permission denied"))
                   || lower.contains(QLatin1String("publickey"))
                   || lower.contains(QLatin1String("authentication failed"))) {
            level = QStringLiteral("authfail");
        } else {
            level = QStringLiteral("unknown");
        }

        Q_EMIT finished(level, summary);
        setRunning(false);
        m_proc->deleteLater();
        m_proc = nullptr;
    });

    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (!m_running) {
            return;
        }
        Q_EMIT finished(QStringLiteral("unreachable"), QStringLiteral("Could not run ssh"));
        setRunning(false);
        if (m_proc) {
            m_proc->deleteLater();
            m_proc = nullptr;
        }
    });

    // Build the command from the live fields, overriding any saved config so
    // the test reflects what is on screen rather than the stored Host block.
    QStringList args{QStringLiteral("-T"),
                     QStringLiteral("-o"), QStringLiteral("BatchMode=yes"),
                     QStringLiteral("-o"), QStringLiteral("ConnectTimeout=10")};

    const QString key = identityFile.trimmed();
    if (!key.isEmpty()) {
        QString path = key;
        if (path == QLatin1String("~")) {
            path = QDir::homePath();
        } else if (path.startsWith(QLatin1String("~/"))) {
            path = QDir::homePath() + path.mid(1);
        }
        args << QStringLiteral("-o") << QStringLiteral("IdentitiesOnly=yes")
             << QStringLiteral("-i") << path;
    }

    const QString p = port.trimmed();
    if (!p.isEmpty() && p != QLatin1String("22")) {
        args << QStringLiteral("-p") << p;
    }

    const QString u = user.trimmed().isEmpty() ? QStringLiteral("git") : user.trimmed();
    args << (u + QLatin1Char('@') + host);

    m_proc->start(QStringLiteral("ssh"), args);
}
