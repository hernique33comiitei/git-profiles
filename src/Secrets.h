#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

// Stores provider tokens in the system secret service (KWallet / libsecret) via
// `secret-tool`, keyed by { service=gitprofiles, profile=<host> }. The generated
// .envrc reads them back with the same key.
class Secrets : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool available READ available CONSTANT)

public:
    explicit Secrets(QObject *parent = nullptr);

    bool available() const;

    Q_INVOKABLE bool hasToken(const QString &profile) const;
    Q_INVOKABLE QString setToken(const QString &profile, const QString &token); // "" on success
    Q_INVOKABLE QString clearToken(const QString &profile); // "" on success

    static QString getToken(const QString &profile);
    static QString secretToolPath();
};
