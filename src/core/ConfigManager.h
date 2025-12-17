/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef QUANTILYX_CONFIGMANAGER_H
#define QUANTILYX_CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QColor>
#include <memory>

namespace QuantilyxDoc {

/**
 * @brief Configuration manager - Singleton pattern
 * 
 * Manages all application configuration stored in INI format.
 * Provides type-safe access to 500+ configuration options.
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     * @return Reference to ConfigManager
     */
    static ConfigManager& instance();

    /**
     * @brief Destructor
     */
    ~ConfigManager();

    /**
     * @brief Load default configuration
     */
    void loadDefaults();

    /**
     * @brief Load configuration from file
     * @param filePath Path to INI file
     * @return true if loaded successfully
     */
    bool loadFromFile(const QString& filePath = QString());

    /**
     * @brief Save configuration to file
     * @param filePath Path to INI file
     * @return true if saved successfully
     */
    bool saveToFile(const QString& filePath = QString());

    /**
     * @brief Import configuration from file
     * @param filePath Path to INI file
     * @return true if imported successfully
     */
    bool importFrom(const QString& filePath);

    /**
     * @brief Export configuration to file
     * @param filePath Path to INI file
     * @return true if exported successfully
     */
    bool exportTo(const QString& filePath);

    /**
     * @brief Get string value
     * @param section Section name
     * @param key Key name
     * @param defaultValue Default value if not found
     * @return String value
     */
    QString getString(const QString& section, const QString& key, 
                     const QString& defaultValue = QString()) const;

    /**
     * @brief Get integer value
     * @param section Section name
     * @param key Key name
     * @param defaultValue Default value if not found
     * @return Integer value
     */
    int getInt(const QString& section, const QString& key, int defaultValue = 0) const;

    /**
     * @brief Get boolean value
     * @param section Section name
     * @param key Key name
     * @param defaultValue Default value if not found
     * @return Boolean value
     */
    bool getBool(const QString& section, const QString& key, bool defaultValue = false) const;

    /**
     * @brief Get double value
     * @param section Section name
     * @param key Key name
     * @param defaultValue Default value if not found
     * @return Double value
     */
    double getDouble(const QString& section, const QString& key, double defaultValue = 0.0) const;

    /**
     * @brief Get color value
     * @param section Section name
     * @param key Key name
     * @param defaultValue Default value if not found
     * @return QColor value
     */
    QColor getColor(const QString& section, const QString& key, 
                   const QColor& defaultValue = QColor()) const;

    /**
     * @brief Get string list value
     * @param section Section name
     * @param key Key name
     * @param defaultValue Default value if not found
     * @return QStringList value
     */
    QStringList getStringList(const QString& section, const QString& key,
                             const QStringList& defaultValue = QStringList()) const;

    /**
     * @brief Set string value
     * @param section Section name
     * @param key Key name
     * @param value Value to set
     */
    void setString(const QString& section, const QString& key, const QString& value);

    /**
     * @brief Set integer value
     * @param section Section name
     * @param key Key name
     * @param value Value to set
     */
    void setInt(const QString& section, const QString& key, int value);

    /**
     * @brief Set boolean value
     * @param section Section name
     * @param key Key name
     * @param value Value to set
     */
    void setBool(const QString& section, const QString& key, bool value);

    /**
     * @brief Set double value
     * @param section Section name
     * @param key Key name
     * @param value Value to set
     */
    void setDouble(const QString& section, const QString& key, double value);

    /**
     * @brief Set color value
     * @param section Section name
     * @param key Key name
     * @param value Value to set
     */
    void setColor(const QString& section, const QString& key, const QColor& value);

    /**
     * @brief Set string list value
     * @param section Section name
     * @param key Key name
     * @param value Value to set
     */
    void setStringList(const QString& section, const QString& key, const QStringList& value);

    /**
     * @brief Check if key exists
     * @param section Section name
     * @param key Key name
     * @return true if key exists
     */
    bool contains(const QString& section, const QString& key) const;

    /**
     * @brief Remove a key
     * @param section Section name
     * @param key Key name
     */
    void remove(const QString& section, const QString& key);

    /**
     * @brief Get all sections
     * @return List of section names
     */
    QStringList sections() const;

    /**
     * @brief Get all keys in a section
     * @param section Section name
     * @return List of key names
     */
    QStringList keys(const QString& section) const;

    /**
     * @brief Reset to default configuration
     */
    void resetToDefaults();

    /**
     * @brief Get configuration file path
     * @return Path to configuration file
     */
    QString configFilePath() const;

    /**
     * @brief Get last error message
     * @return Error message
     */
    QString lastError() const;

signals:
    /**
     * @brief Emitted when configuration changes
     * @param section Section name
     * @param key Key name
     */
    void configChanged(const QString& section, const QString& key);

    /**
     * @brief Emitted when configuration is loaded
     */
    void configLoaded();

    /**
     * @brief Emitted when configuration is saved
     */
    void configSaved();

private:
    /**
     * @brief Private constructor (Singleton)
     */
    ConfigManager();

    /**
     * @brief Disable copy constructor
     */
    ConfigManager(const ConfigManager&) = delete;

    /**
     * @brief Disable assignment operator
     */
    ConfigManager& operator=(const ConfigManager&) = delete;

    /**
     * @brief Initialize default values
     */
    void initializeDefaults();

    /**
     * @brief Get value as QVariant
     * @param section Section name
     * @param key Key name
     * @param defaultValue Default value
     * @return QVariant value
     */
    QVariant getValue(const QString& section, const QString& key, 
                     const QVariant& defaultValue) const;

    /**
     * @brief Set value as QVariant
     * @param section Section name
     * @param key Key name
     * @param value Value to set
     */
    void setValue(const QString& section, const QString& key, const QVariant& value);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_CONFIGMANAGER_H