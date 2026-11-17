#pragma once

#include <QObject>
#include <QProcess>
#include <QQmlEngine>
#include <QString>

// Runs `ssh -T <host alias>` asynchronously (BatchMode, short timeout) so the UI
// never blocks, and reports whether the key authenticates / the host answers.
class SshTester : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit SshTester(QObject *parent = nullptr);

    bool running() const { return m_running; }

    Q_INVOKABLE void test(const QString &hostAlias);

Q_SIGNALS:
    // level: "success" | "authfail" | "unreachable" | "unknown"
    void finished(const QString &level, const QString &message);
    void runningChanged();

private:
    void setRunning(bool r);

    QProcess *m_proc = nullptr;
    bool m_running = false;
};
