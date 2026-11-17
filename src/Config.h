#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSettings>
#include <QString>

#ifndef GITPROFILES_VERSION
#define GITPROFILES_VERSION "0.0.0"
#endif

// Persistent app settings (QSettings). Currently the UI language override and the
// app version (single-sourced from the CMake project version).
class Config : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit Config(QObject *parent = nullptr);

    QString language() const;      // "" = system, "en", "pt_BR"
    void setLanguage(const QString &lang);
    QString version() const;

    // Reads the stored language without an instance (used at startup).
    static QString storedLanguage();

Q_SIGNALS:
    void languageChanged();

private:
    QSettings m_settings;
};
