/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_SETTINGS_H
#define QUANTILYX_SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QHash>
#include <QVariant>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

/**
 * @brief Centralized settings manager.
 *
 * Extends QSettings with type-safe getters/setters and provides
 * a mechanism for registering default values and reacting to changes.
 * Serves as a unified interface for all application preferences.
 */
class Settings : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit Settings(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~Settings() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global Settings instance.
     */
    static Settings& instance();

    /**
     * @brief Register a setting with its default value and type.
     * @param key Setting key (e.g., "Display/BackgroundColor").
     * @param defaultValue Default value for the setting.
     * @param description Optional human-readable description.
     * @param category Optional category for organization (e.g., "Display", "Editor").
     * @return True if registration was successful.
     */
    bool registerSetting(const QString& key, const QVariant& defaultValue,
                         const QString& description = QString(),
                         const QString& category = QString());

    /**
     * @brief Check if a setting is registered.
     * @param key Setting key.
     * @return True if the key is registered.
     */
    bool isRegistered(const QString& key) const;

    /**
     * @brief Get the value of a setting.
     * If the setting is not found or has an incorrect type, the registered default is returned.
     * @tparam T Expected type of the value.
     * @param key Setting key.
     * @return Value of the setting or its default.
     */
    template<typename T>
    T value(const QString& key) const;

    /**
     * @brief Get the value of a setting with a fallback default.
     * @tparam T Expected type of the value.
     * @param key Setting key.
     * @param fallbackValue Value to return if key is not found or type mismatch occurs.
     * @return Value of the setting or the fallback value.
     */
    template<typename T>
    T value(const QString& key, const T& fallbackValue) const;

    /**
     * @brief Set the value of a setting.
     * @tparam T Type of the value.
     * @param key Setting key.
     * @param value New value for the setting.
     */
    template<typename T>
    void setValue(const QString& key, const T& value);

    /**
     * @brief Remove a setting.
     * @param key Setting key to remove.
     */
    void remove(const QString& key);

    /**
     * @brief Check if a setting has a value stored (not just default).
     * @param key Setting key.
     * @return True if the setting has a stored value.
     */
    bool contains(const QString& key) const;

    /**
     * @brief Get all registered setting keys.
     * @return List of keys.
     */
    QStringList allKeys() const;

    /**
     * @brief Get all registered setting keys for a specific category.
     * @param category Category name.
     * @return List of keys in the category.
     */
    QStringList keysInCategory(const QString& category) const;

    /**
     * @brief Get the default value for a registered setting.
     * @param key Setting key.
     * @return Registered default value.
     */
    QVariant defaultValue(const QString& key) const;

    /**
     * @brief Get the description for a registered setting.
     * @param key Setting key.
     * @return Description string.
     */
    QString description(const QString& key) const;

    /**
     * @brief Get the category for a registered setting.
     * @param key Setting key.
     * @return Category string.
     */
    QString category(const QString& key) const;

    /**
     * @brief Reset a specific setting to its default value.
     * @param key Setting key to reset.
     */
    void resetToDefault(const QString& key);

    /**
     * @brief Reset all settings to their default values.
     */
    void resetAllToDefaults();

    /**
     * @brief Reload settings from persistent storage.
     * Useful if settings might have been changed externally.
     */
    void reload();

    /**
     * @brief Save current settings to persistent storage.
     */
    void save();

    /**
     * @brief Get the underlying QSettings object.
     * @return Pointer to the QSettings instance.
     */
    QSettings* qsettings() const;

    /**
     * @brief Register a callback to be notified when a specific setting changes.
     * @param key Setting key to watch.
     * @param callback Function to call when the setting changes.
     * @return Connection ID for potential disconnection.
     */
    QMetaObject::Connection registerChangeCallback(const QString& key,
                                                   std::function<void(const QVariant&)> callback);

    /**
     * @brief Unregister a previously registered change callback.
     * @param key Setting key.
     * @param connectionId Connection ID returned by registerChangeCallback.
     */
    void unregisterChangeCallback(const QString& key, QMetaObject::Connection connectionId);

signals:
    /**
     * @brief Emitted when any setting value changes.
     * @param key The key of the changed setting.
     * @param newValue The new value.
     */
    void valueChanged(const QString& key, const QVariant& newValue);

    /**
     * @brief Emitted when settings are reloaded from storage.
     */
    void reloaded();

    /**
     * @brief Emitted when settings are saved to storage.
     */
    void saved();

private:
    class Private;
    std::unique_ptr<Private> d;
};

// Inline template implementations
template<typename T>
T Settings::value(const QString& key) const {
    QVariant storedValue = qsettings()->value(key);
    if (storedValue.isValid() && storedValue.canConvert<T>()) {
        return storedValue.value<T>();
    }
    // If stored value is invalid or wrong type, return registered default
    QVariant defaultValue = this->defaultValue(key);
    if (defaultValue.isValid() && defaultValue.canConvert<T>()) {
        return defaultValue.value<T>();
    }
    // If no default is registered or wrong type, return a default-constructed T
    return T{};
}

template<typename T>
T Settings::value(const QString& key, const T& fallbackValue) const {
    QVariant storedValue = qsettings()->value(key);
    if (storedValue.isValid() && storedValue.canConvert<T>()) {
        return storedValue.value<T>();
    }
    return fallbackValue;
}

template<typename T>
void Settings::setValue(const QString& key, const T& value) {
    if (qsettings()->value(key) != QVariant(value)) { // Only emit if value actually changes
        qsettings()->setValue(key, value);
        emit valueChanged(key, QVariant(value));
    }
}


} // namespace QuantilyxDoc

#endif // QUANTILYX_SETTINGS_H