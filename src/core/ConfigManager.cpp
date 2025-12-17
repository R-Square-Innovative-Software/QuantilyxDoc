/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "ConfigManager.h"
#include "Logger.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QColor>

namespace QuantilyxDoc {

// Private implementation
class ConfigManager::Private
{
public:
    Private() : settings(nullptr) {}
    
    ~Private() {
        delete settings;
    }

    QSettings* settings;
    QString configPath;
    QString lastError;
    
    // Default configuration file name
    static const QString CONFIG_FILENAME;
};

const QString ConfigManager::Private::CONFIG_FILENAME = "quantilyxdoc.ini";

ConfigManager::ConfigManager()
    : QObject(nullptr)
    , d(new Private())
{
    // Determine configuration file path
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + 
                       "/quantilyxdoc";
    
    // Create directory if it doesn't exist
    QDir().mkpath(configDir);
    
    d->configPath = configDir + "/" + Private::CONFIG_FILENAME;
    
    // Create QSettings instance
    d->settings = new QSettings(d->configPath, QSettings::IniFormat);
    
    LOG_INFO("ConfigManager initialized with file: " << d->configPath);
}

ConfigManager::~ConfigManager()
{
    LOG_INFO("ConfigManager destroyed");
}

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

void ConfigManager::loadDefaults()
{
    LOG_INFO("Loading default configuration...");
    initializeDefaults();
    LOG_INFO("Default configuration loaded");
}

bool ConfigManager::loadFromFile(const QString& filePath)
{
    QString path = filePath.isEmpty() ? d->configPath : filePath;
    
    LOG_INFO("Loading configuration from: " << path);
    
    if (!QFile::exists(path)) {
        d->lastError = "Configuration file does not exist: " + path;
        LOG_WARNING(d->lastError);
        
        // Load defaults instead
        initializeDefaults();
        return false;
    }
    
    delete d->settings;
    d->settings = new QSettings(path, QSettings::IniFormat);
    
    if (d->settings->status() != QSettings::NoError) {
        d->lastError = "Failed to load configuration file: " + path;
        LOG_ERROR(d->lastError);
        return false;
    }
    
    emit configLoaded();
    LOG_INFO("Configuration loaded successfully");
    return true;
}

bool ConfigManager::saveToFile(const QString& filePath)
{
    QString path = filePath.isEmpty() ? d->configPath : filePath;
    
    LOG_INFO("Saving configuration to: " << path);
    
    d->settings->sync();
    
    if (d->settings->status() != QSettings::NoError) {
        d->lastError = "Failed to save configuration file: " + path;
        LOG_ERROR(d->lastError);
        return false;
    }
    
    emit configSaved();
    LOG_INFO("Configuration saved successfully");
    return true;
}

bool ConfigManager::importFrom(const QString& filePath)
{
    LOG_INFO("Importing configuration from: " << filePath);
    
    if (!QFile::exists(filePath)) {
        d->lastError = "Import file does not exist: " + filePath;
        LOG_ERROR(d->lastError);
        return false;
    }
    
    QSettings importSettings(filePath, QSettings::IniFormat);
    
    if (importSettings.status() != QSettings::NoError) {
        d->lastError = "Failed to read import file: " + filePath;
        LOG_ERROR(d->lastError);
        return false;
    }
    
    // Copy all settings
    QStringList groups = importSettings.childGroups();
    for (const QString& group : groups) {
        importSettings.beginGroup(group);
        QStringList keys = importSettings.childKeys();
        
        d->settings->beginGroup(group);
        for (const QString& key : keys) {
            d->settings->setValue(key, importSettings.value(key));
        }
        d->settings->endGroup();
        
        importSettings.endGroup();
    }
    
    d->settings->sync();
    
    LOG_INFO("Configuration imported successfully");
    return true;
}

bool ConfigManager::exportTo(const QString& filePath)
{
    LOG_INFO("Exporting configuration to: " << filePath);
    
    // Save current settings first
    d->settings->sync();
    
    // Copy file
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
    }
    
    if (!QFile::copy(d->configPath, filePath)) {
        d->lastError = "Failed to export configuration to: " + filePath;
        LOG_ERROR(d->lastError);
        return false;
    }
    
    LOG_INFO("Configuration exported successfully");
    return true;
}

QString ConfigManager::getString(const QString& section, const QString& key, 
                                const QString& defaultValue) const
{
    return getValue(section, key, defaultValue).toString();
}

int ConfigManager::getInt(const QString& section, const QString& key, int defaultValue) const
{
    return getValue(section, key, defaultValue).toInt();
}

bool ConfigManager::getBool(const QString& section, const QString& key, bool defaultValue) const
{
    return getValue(section, key, defaultValue).toBool();
}

double ConfigManager::getDouble(const QString& section, const QString& key, 
                               double defaultValue) const
{
    return getValue(section, key, defaultValue).toDouble();
}

QColor ConfigManager::getColor(const QString& section, const QString& key, 
                              const QColor& defaultValue) const
{
    QString colorStr = getString(section, key, defaultValue.name());
    return QColor(colorStr);
}

QStringList ConfigManager::getStringList(const QString& section, const QString& key,
                                        const QStringList& defaultValue) const
{
    QString str = getString(section, key, defaultValue.join(","));
    if (str.isEmpty()) {
        return QStringList();
    }
    return str.split(",", Qt::SkipEmptyParts);
}

void ConfigManager::setString(const QString& section, const QString& key, const QString& value)
{
    setValue(section, key, value);
}

void ConfigManager::setInt(const QString& section, const QString& key, int value)
{
    setValue(section, key, value);
}

void ConfigManager::setBool(const QString& section, const QString& key, bool value)
{
    setValue(section, key, value);
}

void ConfigManager::setDouble(const QString& section, const QString& key, double value)
{
    setValue(section, key, value);
}

void ConfigManager::setColor(const QString& section, const QString& key, const QColor& value)
{
    setValue(section, key, value.name());
}

void ConfigManager::setStringList(const QString& section, const QString& key, 
                                 const QStringList& value)
{
    setValue(section, key, value.join(","));
}

bool ConfigManager::contains(const QString& section, const QString& key) const
{
    d->settings->beginGroup(section);
    bool result = d->settings->contains(key);
    d->settings->endGroup();
    return result;
}

void ConfigManager::remove(const QString& section, const QString& key)
{
    d->settings->beginGroup(section);
    d->settings->remove(key);
    d->settings->endGroup();
    
    emit configChanged(section, key);
}

QStringList ConfigManager::sections() const
{
    return d->settings->childGroups();
}

QStringList ConfigManager::keys(const QString& section) const
{
    d->settings->beginGroup(section);
    QStringList keysList = d->settings->childKeys();
    d->settings->endGroup();
    return keysList;
}

void ConfigManager::resetToDefaults()
{
    LOG_INFO("Resetting configuration to defaults...");
    
    d->settings->clear();
    initializeDefaults();
    d->settings->sync();
    
    LOG_INFO("Configuration reset to defaults");
    emit configLoaded();
}

QString ConfigManager::configFilePath() const
{
    return d->configPath;
}

QString ConfigManager::lastError() const
{
    return d->lastError;
}

QVariant ConfigManager::getValue(const QString& section, const QString& key, 
                                const QVariant& defaultValue) const
{
    d->settings->beginGroup(section);
    QVariant value = d->settings->value(key, defaultValue);
    d->settings->endGroup();
    return value;
}

void ConfigManager::setValue(const QString& section, const QString& key, const QVariant& value)
{
    d->settings->beginGroup(section);
    d->settings->setValue(key, value);
    d->settings->endGroup();
    
    emit configChanged(section, key);
}

void ConfigManager::initializeDefaults()
{
    LOG_INFO("Initializing default configuration values...");
    
    // Only set if not already present
    auto setDefault = [this](const QString& section, const QString& key, const QVariant& value) {
        if (!contains(section, key)) {
            setValue(section, key, value);
        }
    };
    
    // [General] section (17 settings)
    setDefault("General", "language", "en");
    setDefault("General", "check_updates", false);
    setDefault("General", "show_splash", true);
    setDefault("General", "splash_timeout", 3000);
    setDefault("General", "single_instance", true);
    setDefault("General", "restore_session", true);
    setDefault("General", "remember_window_state", true);
    setDefault("General", "confirm_quit", true);
    setDefault("General", "recent_files_count", 20);
    setDefault("General", "default_save_location", QDir::homePath() + "/Documents");
    setDefault("General", "auto_save_interval", 300);
    setDefault("General", "backup_before_save", true);
    setDefault("General", "max_backups", 10);
    setDefault("General", "associate_file_types", true);
    setDefault("General", "default_open_action", "tab");
    setDefault("General", "open_blank_document", false);
    setDefault("General", "open_last_document", true);
    
    // [Appearance] section (25 settings)
    setDefault("Appearance", "theme", "auto");
    setDefault("Appearance", "custom_theme_file", "");
    setDefault("Appearance", "icon_theme", "breeze");
    setDefault("Appearance", "icon_size", 22);
    setDefault("Appearance", "use_system_colors", true);
    setDefault("Appearance", "ui_font", "Sans Serif");
    setDefault("Appearance", "ui_font_size", 10);
    setDefault("Appearance", "document_font", "Liberation Sans");
    setDefault("Appearance", "document_font_size", 12);
    setDefault("Appearance", "monospace_font", "Liberation Mono");
    setDefault("Appearance", "monospace_font_size", 10);
    setDefault("Appearance", "show_menubar", true);
    setDefault("Appearance", "show_toolbar", true);
    setDefault("Appearance", "show_statusbar", true);
    setDefault("Appearance", "show_sidebar", true);
    setDefault("Appearance", "show_properties_panel", true);
    setDefault("Appearance", "toolbar_style", "icon_text");
    setDefault("Appearance", "tab_position", "top");
    setDefault("Appearance", "tab_close_button", true);
    setDefault("Appearance", "tab_document_icon", true);
    setDefault("Appearance", "window_opacity", 1.0);
    setDefault("Appearance", "highlight_color", "#3498db");
    setDefault("Appearance", "link_color", "#2980b9");
    setDefault("Appearance", "background_color", "#ffffff");
    setDefault("Appearance", "text_color", "#2c3e50");
    
    // [Performance] section (12 settings)
    setDefault("Performance", "max_memory_usage", 2048);
    setDefault("Performance", "page_cache_size", 50);
    setDefault("Performance", "thumbnail_cache_size", 200);
    setDefault("Performance", "clear_cache_on_exit", false);
    setDefault("Performance", "render_threads", 0);
    setDefault("Performance", "prefetch_pages", true);
    setDefault("Performance", "lazy_loading", true);
    setDefault("Performance", "progressive_rendering", true);
    setDefault("Performance", "use_gpu", "auto");
    setDefault("Performance", "gpu_memory_limit", 512);
    setDefault("Performance", "optimize_memory", true);
    setDefault("Performance", "low_memory_mode", false);
    
    // [Logging] section (15 settings)
    setDefault("Logging", "enable_logging", true);
    setDefault("Logging", "log_level", "info");
    setDefault("Logging", "log_file", "");  // Will use default
    setDefault("Logging", "log_max_size", 10);
    setDefault("Logging", "log_max_files", 5);
    setDefault("Logging", "log_to_console", false);
    setDefault("Logging", "log_timestamps", true);
    setDefault("Logging", "log_thread_id", false);
    setDefault("Logging", "log_function_name", true);
    setDefault("Logging", "debug_mode", false);
    setDefault("Logging", "debug_rendering", false);
    setDefault("Logging", "debug_memory", false);
    setDefault("Logging", "debug_performance", false);
    setDefault("Logging", "enable_crash_reporting", false);
    setDefault("Logging", "create_core_dump", false);
    
    // Add more sections as needed...
    
    LOG_INFO("Default configuration values initialized");
}

} // namespace QuantilyxDoc