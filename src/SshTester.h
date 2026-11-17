#pragma once

#include <QObject>
#include <QProcess>
#include <QQmlEngine>
#include <QString>

// Runs `ssh -T` asynchronously (BatchMode, short timeout) so the UI never
// blocks, and reports whether the key authenticates / the host answers. The
// connection is driven by the fields passed in (not a saved ~/.ssh/config Host
// block), so it tests exactly what is on screen — including unsaved edits.
class SshTester : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit SshTester(QObject *parent = nullptr);

    bool running() const { return m_running; }

    // Tests the given connection parameters directly (ssh -o overrides), so
    // in-editor values are honored without needing to save first. An empty
    // user defaults to "git"; an empty/22 port and empty identityFile are
    // simply omitted.
    Q_INVOKABLE void test(const QString &hostName, const QString &user,
                          const QString &port, const QString &identityFile);

Q_SIGNALS:
    // level: "success" | "authfail" | "unreachable" | "unknown"
    void finished(const QString &level, const QString &message);
    void runningChanged();

private:
    void setRunning(bool r);

    QProcess *m_proc = nullptr;
    bool m_running = false;
};
