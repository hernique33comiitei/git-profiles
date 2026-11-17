#include "SshTester.h"

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

void SshTester::test(const QString &hostAlias)
{
    if (m_running || hostAlias.trimmed().isEmpty()) {
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

    m_proc->start(QStringLiteral("ssh"),
                  {QStringLiteral("-T"), QStringLiteral("-o"), QStringLiteral("BatchMode=yes"),
                   QStringLiteral("-o"), QStringLiteral("ConnectTimeout=10"), hostAlias.trimmed()});
}
