/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef QUANTILYX_APPLICATION_H
#define QUANTILYX_APPLICATION_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <memory>

namespace QuantilyxDoc {

// Forward declarations
class PluginInterface;
class Document;
class MainWindow;

/**
 * @brief Core application class - Singleton pattern
 * 
 * Manages application-wide resources, plugins, and global state.
 */
class Application : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance
     * @return Pointer to the Application instance
     */
    static Application* instance();

    /**
     * @brief Destructor
     */
    ~Application();

    /**
     * @brief Check if another instance is already running
     * @return true if another instance exists
     */
    static bool isAlreadyRunning();

    /**
     * @brief Send files to existing instance
     * @param files List of file paths to open
     * @return true if successfully sent
     */
    static bool sendFilesToExistingInstance(const QStringList& files);

    /**
     * @brief Initialize the application
     * @return true if initialization succeeded
     */
    bool initialize();

    /**
     * @brief Load all plugins
     */
    void loadPlugins();

    /**
     * @brief Initialize OCR engines
     */
    void initializeOCR();

    /**
     * @brief Get list of loaded plugins
     * @return Map of plugin name to plugin interface
     */
    const QMap<QString, PluginInterface*>& plugins() const;

    /**
     * @brief Get plugin by name
     * @param name Plugin name
     * @return Plugin interface or nullptr if not found
     */
    PluginInterface* getPlugin(const QString& name) const;

    /**
     * @brief Check if plugin is loaded
     * @param name Plugin name
     * @return true if plugin is loaded
     */
    bool hasPlugin(const QString& name) const;

    /**
     * @brief Get application version
     * @return Version string
     */
    static QString version();

    /**
     * @brief Get application build date
     * @return Build date string
     */
    static QString buildDate();

    /**
     * @brief Get home page URL
     * @return URL string
     */
    static QString homePage();

    /**
     * @brief Get bug report URL
     * @return URL string
     */
    static QString bugReportUrl();

    /**
     * @brief Get all open documents
     * @return List of document pointers
     */
    QList<Document*> openDocuments() const;

    /**
     * @brief Register a document
     * @param doc Document to register
     */
    void registerDocument(Document* doc);

    /**
     * @brief Unregister a document
     * @param doc Document to unregister
     */
    void unregisterDocument(Document* doc);

    /**
     * @brief Get main window
     * @return Pointer to main window
     */
    MainWindow* mainWindow() const;

    /**
     * @brief Set main window
     * @param window Main window pointer
     */
    void setMainWindow(MainWindow* window);

    /**
     * @brief Get temporary directory
     * @return Temporary directory path
     */
    QString tempDirectory() const;

    /**
     * @brief Get cache directory
     * @return Cache directory path
     */
    QString cacheDirectory() const;

    /**
     * @brief Get configuration directory
     * @return Configuration directory path
     */
    QString configDirectory() const;

    /**
     * @brief Get data directory
     * @return Data directory path
     */
    QString dataDirectory() const;

    /**
     * @brief Get plugins directory
     * @return Plugins directory path
     */
    QString pluginsDirectory() const;

    /**
     * @brief Get translations directory
     * @return Translations directory path
     */
    QString translationsDirectory() const;

    /**
     * @brief Clear all caches
     */
    void clearCaches();

    /**
     * @brief Get OCR engine availability
     * @return Map of engine name to availability
     */
    QMap<QString, bool> ocrEnginesAvailable() const;

signals:
    /**
     * @brief Emitted when a plugin is loaded
     * @param name Plugin name
     */
    void pluginLoaded(const QString& name);

    /**
     * @brief Emitted when a plugin fails to load
     * @param name Plugin name
     * @param error Error message
     */
    void pluginLoadFailed(const QString& name, const QString& error);

    /**
     * @brief Emitted when a document is registered
     * @param doc Document pointer
     */
    void documentRegistered(Document* doc);

    /**
     * @brief Emitted when a document is unregistered
     * @param doc Document pointer
     */
    void documentUnregistered(Document* doc);

    /**
     * @brief Emitted when application is about to quit
     */
    void aboutToQuit();

private:
    /**
     * @brief Private constructor (Singleton)
     */
    Application();

    /**
     * @brief Disable copy constructor
     */
    Application(const Application&) = delete;

    /**
     * @brief Disable assignment operator
     */
    Application& operator=(const Application&) = delete;

    /**
     * @brief Load a single plugin
     * @param pluginPath Path to plugin library
     * @return true if loaded successfully
     */
    bool loadPlugin(const QString& pluginPath);

    /**
     * @brief Initialize directories
     */
    void initializeDirectories();

    /**
     * @brief Setup IPC for single instance
     */
    void setupIPC();

private:
    class Private;
    std::unique_ptr<Private> d;

    static Application* s_instance;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_APPLICATION_H