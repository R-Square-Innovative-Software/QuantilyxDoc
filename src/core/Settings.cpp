/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "Settings.h"
#include "Logger.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaObject>
#include <QCoreApplication>
#include <QDebug>

namespace QuantilyxDoc {

struct RegisteredSetting {
    QVariant defaultValue;
    QString description;
    QString category;
    QHash<QObject*, QMetaObject::Connection> callbacks; // Object -> Connection ID map
};

class Settings::Private {
public:
    Private(Settings* q_ptr)
        : q(q_ptr),
          qtSettings(nullptr) {}

    Settings* q;
    QScopedPointer<QSettings> qtSettings;
    QHash<QString, RegisteredSetting> registeredSettings;
    QMutex mutex; // Protect access to registeredSettings and qtSettings

    // Helper to get or create the QSettings instance
    QSettings* getOrCreateQSettings() {
        if (!qtSettings) {
            // Determine standard location for settings file
            QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
            QDir dir(configDir);
            if (!dir.exists()) {
                dir.mkpath("."); // Create directory if it doesn't exist
            }
            QString settingsPath = dir.filePath("quantilyxdoc.conf");
            qtSettings.reset(new QSettings(settingsPath, QSettings::IniFormat));
            LOG_INFO("Initialized settings file: " << settingsPath);
        }
        return qtSettings.data();
    }
};

// Static instance pointer
Settings* Settings::s_instance = nullptr;

Settings& Settings::instance()
{
    if (!s_instance) {
        s_instance = new Settings();
    }
    return *s_instance;
}

Settings::Settings(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    // QSettings will be created lazily in getOrCreateQSettings()
    // Register some example settings on first instantiation
    if (d->registeredSettings.isEmpty()) {
        registerSetting("Display/BackgroundColor", QColor(Qt::white), "Background color for document view", "Display");
        registerSetting("Display/UseHighDpiPixmaps", true, "Enable high DPI pixmap scaling", "Display");
        registerSetting("Editor/AutoIndent", true, "Automatically indent new lines", "Editor");
        registerSetting("Editor/TabWidth", 4, "Number of spaces per tab character", "Editor");
        registerSetting("General/Language", "en_US", "Application language (e.g., en_US, fr_FR)", "General");
        registerSetting("General/CheckForUpdates", true, "Automatically check for application updates", "General");
        LOG_DEBUG("Registered default settings.");
    }
}

Settings::~Settings()
{
    save(); // Ensure settings are saved on destruction
    // Connections will be automatically disconnected by QObject parent-child mechanism or when objects are destroyed
}

bool Settings::registerSetting(const QString& key, const QVariant& defaultValue,
                               const QString& description, const QString& category)
{
    QMutexLocker locker(&d->mutex);
    if (d->registeredSettings.contains(key)) {
        LOG_WARN("Setting key '" << key << "' is already registered. Skipping registration.");
        return false;
    }

    RegisteredSetting reg;
    reg.defaultValue = defaultValue;
    reg.description = description;
    reg.category = category;
    d->registeredSettings.insert(key, reg);

    LOG_DEBUG("Registered setting: " << key << " (default: " << defaultValue.toString() << ")");
    return true;
}

bool Settings::isRegistered(const QString& key) const
{
    QMutexLocker locker(&d->mutex);
    return d->registeredSettings.contains(key);
}

void Settings::remove(const QString& key)
{
    QMutexLocker locker(&d->mutex);
    if (d->getOrCreateQSettings()->contains(key)) {
        d->getOrCreateQSettings()->remove(key);
        LOG_DEBUG("Removed setting: " << key);
        // Note: This does NOT emit valueChanged, as the value isn't changing to a new value, it's being removed.
        // The application should handle removal explicitly if needed.
    }
}

bool Settings::contains(const QString& key) const
{
    QMutexLocker locker(&d->mutex);
    return d->getOrCreateQSettings()->contains(key);
}

QStringList Settings::allKeys() const
{
    QMutexLocker locker(&d->mutex);
    return d->getOrCreateQSettings()->allKeys();
}

QStringList Settings::keysInCategory(const QString& category) const
{
    QMutexLocker locker(&d->mutex);
    QStringList all = d->getOrCreateQSettings()->allKeys();
    QStringList filtered;
    for (const QString& key : all) {
        if (key.startsWith(category + "/")) {
            filtered.append(key);
        }
    }
    return filtered;
}

QVariant Settings::defaultValue(const QString& key) const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->registeredSettings.constFind(key);
    if (it != d->registeredSettings.constEnd()) {
        return it->defaultValue;
    }
    return QVariant();
}

QString Settings::description(const QString& key) const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->registeredSettings.constFind(key);
    if (it != d->registeredSettings.constEnd()) {
        return it->description;
    }
    return QString();
}

QString Settings::category(const QString& key) const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->registeredSettings.constFind(key);
    if (it != d->registeredSettings.constEnd()) {
        return it->category;
    }
    return QString();
}

void Settings::resetToDefault(const QString& key)
{
    QMutexLocker locker(&d->mutex);
    auto it = d->registeredSettings.constFind(key);
    if (it != d->registeredSettings.constEnd()) {
        if (d->getOrCreateQSettings()->value(key) != it->defaultValue) {
            d->getOrCreateQSettings()->setValue(key, it->defaultValue);
            LOG_DEBUG("Reset setting " << key << " to default: " << it->defaultValue.toString());
            emit valueChanged(key, it->defaultValue);
        }
    } else {
        LOG_WARN("Cannot reset unregistered setting: " << key);
    }
}

void Settings::resetAllToDefaults()
{
    QMutexLocker locker(&d->mutex);
    bool anyChanged = false;
    for (auto it = d->registeredSettings.constBegin(); it != d->registeredSettings.constEnd(); ++it) {
        const QString& key = it.key();
        const QVariant& defaultValue = it->defaultValue;
        if (d->getOrCreateQSettings()->value(key) != defaultValue) {
            d->getOrCreateQSettings()->setValue(key, defaultValue);
            anyChanged = true;
            // Do not emit here, emit once after the loop to prevent spamming
        }
    }
    if (anyChanged) {
        LOG_INFO("Reset all settings to defaults.");
        // Emit a general change signal or iterate again to emit specific ones if needed.
        // For simplicity, we'll emit a generic signal or rely on UI to refresh entirely.
        // A more granular approach would require storing original values or iterating again.
        // emit settingsReset(); // Add this signal if needed.
    }
}

void Settings::reload()
{
    QMutexLocker locker(&d->mutex);
    if (d->qtSettings) {
        d->qtSettings->sync(); // Flush writes
        d->qtSettings->readIniFile(d->qtSettings->fileName()); // Re-read from file
        LOG_INFO("Reloaded settings from file.");
        emit reloaded();
    }
}

void Settings::save()
{
    QMutexLocker locker(&d->mutex);
    if (d->qtSettings) {
        d->qtSettings->sync(); // Force write to disk
        LOG_INFO("Saved settings to file.");
        emit saved();
    }
}

QSettings* Settings::qsettings() const
{
    QMutexLocker locker(&d->mutex);
    return d->getOrCreateQSettings();
}

QMetaObject::Connection Settings::registerChangeCallback(const QString& key,
                                                         std::function<void(const QVariant&)> callback)
{
    // This is a simplified approach. A more robust system might use QObject signals/slots internally.
    // For now, we connect to our own 'valueChanged' signal and filter.
    // This requires the callback to be wrapped in a lambda that captures the key and filter.
    QObject* callbackHolder = new QObject(); // Needs a parent or manual deletion management
    callbackHolder->setParent(this); // Attach to Settings lifetime
    return connect(this, &Settings::valueChanged, callbackHolder,
                   [callback, key](const QString& changedKey, const QVariant& newValue) {
                       if (changedKey == key) {
                           callback(newValue);
                       }
                   });
}

void Settings::unregisterChangeCallback(const QString& key, QMetaObject::Connection connectionId)
{
    // Disconnecting arbitrary connections is tricky with QMetaObject::Connection.
    // A better design might manage callbacks differently (e.g., using IDs).
    // For now, we rely on the callbackHolder's parent-child relationship for cleanup.
    Q_UNUSED(key);
    QObject::disconnect(connectionId);
    // The callbackHolder will be deleted when Settings is destroyed, disconnecting the signal.
}

} // namespace QuantilyxDoc