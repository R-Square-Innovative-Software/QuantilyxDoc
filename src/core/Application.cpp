/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Application.h"
#include "Logger.h"
#include "ConfigManager.h"
#include "Document.h"
#include "../ui/MainWindow.h"
#include "../plugins/PluginInterface.h"
#include "utils/Version.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QPluginLoader>
#include <QLocalSocket>
#include <QLocalServer>
#include <QDataStream>
#include <QCoreApplication>

namespace QuantilyxDoc {

// Private implementation
class Application::Private
{
public:
    Private() : mainWindow(nullptr), localServer(nullptr) {}

    MainWindow* mainWindow;
    QMap<QString, PluginInterface*> plugins;
    QList<Document*> documents;
    QLocalServer* localServer;
    
    QString tempDir;
    QString cacheDir;
    QString configDir;
    QString dataDir;
    QString pluginsDir;
    QString translationsDir;
    
    QMap<QString, bool> ocrEngines;
    
    static const QString IPC_SERVER_NAME;
};

const QString Application::Private::IPC_SERVER_NAME = "quantilyxdoc-ipc";

// Static instance
Application* Application::s_instance = nullptr;

Application::Application()
    : QObject(nullptr)
    , d(new Private())
{
    LOG_INFO("Application instance created");
    initializeDirectories();
    setupIPC();
}

Application::~Application()
{
    LOG_INFO("Application instance destroyed");
    
    emit aboutToQuit();
    
    // Cleanup plugins
    for (auto* plugin : d->plugins.values()) {
        delete plugin;
    }
    d->plugins.clear();
    
    // Cleanup documents
    qDeleteAll(d->documents);
    d->documents.clear();
    
    // Cleanup IPC
    if (d->localServer) {
        d->localServer->close();
        delete d->localServer;
    }
}

Application* Application::instance()
{
    if (!s_instance) {
        s_instance = new Application();
    }
    return s_instance;
}

bool Application::isAlreadyRunning()
{
    QLocalSocket socket;
    socket.connectToServer(Private::IPC_SERVER_NAME);
    
    if (socket.waitForConnected(500)) {
        socket.disconnectFromServer();
        return true;
    }
    
    return false;
}

bool Application::sendFilesToExistingInstance(const QStringList& files)
{
    if (files.isEmpty()) {
        return false;
    }
    
    QLocalSocket socket;
    socket.connectToServer(Private::IPC_SERVER_NAME);
    
    if (!socket.waitForConnected(1000)) {
        LOG_ERROR("Failed to connect to existing instance");
        return false;
    }
    
    // Send files list
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << files;
    
    socket.write(data);
    socket.flush();
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    
    LOG_INFO("Sent " << files.size() << " files to existing instance");
    return true;
}

bool Application::initialize()
{
    LOG_INFO("Initializing application...");
    
    // Create necessary directories
    QDir().mkpath(d->tempDir);
    QDir().mkpath(d->cacheDir);
    QDir().mkpath(d->configDir);
    QDir().mkpath(d->dataDir);
    QDir().mkpath(d->pluginsDir);
    
    LOG_INFO("Application initialized successfully");
    return true;
}

void Application::loadPlugins()
{
    LOG_INFO("Loading plugins from: " << d->pluginsDir);
    
    ConfigManager& config = ConfigManager::instance();
    bool autoLoad = config.getBool("Plugins", "auto_load_plugins", true);
    
    if (!autoLoad) {
        LOG_INFO("Plugin auto-loading disabled");
        return;
    }
    
    QDir pluginsDir(d->pluginsDir);
    QStringList pluginFiles = pluginsDir.entryList(QDir::Files);
    
    int loadedCount = 0;
    int failedCount = 0;
    
    for (const QString& fileName : pluginFiles) {
        if (!fileName.endsWith(".so") && !fileName.endsWith(".dll") && !fileName.endsWith(".dylib")) {
            continue;
        }
        
        QString pluginPath = pluginsDir.absoluteFilePath(fileName);
        if (loadPlugin(pluginPath)) {
            loadedCount++;
        } else {
            failedCount++;
        }
    }
    
    LOG_INFO("Loaded " << loadedCount << " plugins, " << failedCount << " failed");
}

bool Application::loadPlugin(const QString& pluginPath)
{
    QPluginLoader loader(pluginPath);
    QObject* pluginObj = loader.instance();
    
    if (!pluginObj) {
        QString error = loader.errorString();
        LOG_ERROR("Failed to load plugin: " << pluginPath << " - " << error);
        emit pluginLoadFailed(pluginPath, error);
        return false;
    }
    
    PluginInterface* plugin = qobject_cast<PluginInterface*>(pluginObj);
    if (!plugin) {
        LOG_ERROR("Plugin does not implement PluginInterface: " << pluginPath);
        emit pluginLoadFailed(pluginPath, "Invalid plugin interface");
        return false;
    }
    
    QString pluginName = plugin->name();
    
    // Check if plugin is enabled
    ConfigManager& config = ConfigManager::instance();
    QStringList enabledPlugins = config.getString("Plugins", "enabled_plugins", "").split(',');
    
    if (!enabledPlugins.isEmpty() && !enabledPlugins.contains(pluginName)) {
        LOG_INFO("Plugin " << pluginName << " is disabled in configuration");
        return false;
    }
    
    // Initialize plugin
    if (!plugin->initialize(this)) {
        LOG_ERROR("Plugin initialization failed: " << pluginName);
        emit pluginLoadFailed(pluginName, "Initialization failed");
        return false;
    }
    
    d->plugins.insert(pluginName, plugin);
    LOG_INFO("Plugin loaded: " << pluginName << " v" << plugin->version());
    emit pluginLoaded(pluginName);
    
    return true;
}

void Application::initializeOCR()
{
    LOG_INFO("Initializing OCR engines...");
    
    d->ocrEngines.clear();
    
#ifdef HAVE_TESSERACT
    d->ocrEngines.insert("Tesseract", true);
    LOG_INFO("Tesseract OCR available");
#else
    d->ocrEngines.insert("Tesseract", false);
#endif

#ifdef HAVE_PADDLEOCR
    d->ocrEngines.insert("PaddleOCR", true);
    LOG_INFO("PaddleOCR available");
#else
    d->ocrEngines.insert("PaddleOCR", false);
#endif
    
    if (d->ocrEngines.values().contains(true)) {
        LOG_INFO("OCR engines initialized successfully");
    } else {
        LOG_WARNING("No OCR engines available");
    }
}

const QMap<QString, PluginInterface*>& Application::plugins() const
{
    return d->plugins;
}

PluginInterface* Application::getPlugin(const QString& name) const
{
    return d->plugins.value(name, nullptr);
}

bool Application::hasPlugin(const QString& name) const
{
    return d->plugins.contains(name);
}

QString Application::version()
{
    return QUANTILYXDOC_VERSION_STRING;
}

QString Application::buildDate()
{
    return __DATE__ " " __TIME__;
}

QString Application::homePage()
{
    return "https://github.com/R-Square-Innovative-Software/QuantilyxDoc";
}

QString Application::bugReportUrl()
{
    return "https://github.com/R-Square-Innovative-Software/QuantilyxDoc/issues";
}

QList<Document*> Application::openDocuments() const
{
    return d->documents;
}

void Application::registerDocument(Document* doc)
{
    if (!d->documents.contains(doc)) {
        d->documents.append(doc);
        LOG_INFO("Document registered: " << doc->filePath());
        emit documentRegistered(doc);
    }
}

void Application::unregisterDocument(Document* doc)
{
    if (d->documents.removeOne(doc)) {
        LOG_INFO("Document unregistered: " << doc->filePath());
        emit documentUnregistered(doc);
    }
}

MainWindow* Application::mainWindow() const
{
    return d->mainWindow;
}

void Application::setMainWindow(MainWindow* window)
{
    d->mainWindow = window;
}

QString Application::tempDirectory() const
{
    return d->tempDir;
}

QString Application::cacheDirectory() const
{
    return d->cacheDir;
}

QString Application::configDirectory() const
{
    return d->configDir;
}

QString Application::dataDirectory() const
{
    return d->dataDir;
}

QString Application::pluginsDirectory() const
{
    return d->pluginsDir;
}

QString Application::translationsDirectory() const
{
    return d->translationsDir;
}

void Application::clearCaches()
{
    LOG_INFO("Clearing all caches...");
    
    QDir cacheDir(d->cacheDir);
    cacheDir.removeRecursively();
    cacheDir.mkpath(".");
    
    LOG_INFO("Caches cleared");
}

QMap<QString, bool> Application::ocrEnginesAvailable() const
{
    return d->ocrEngines;
}

void Application::initializeDirectories()
{
    // Temporary directory
    d->tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + 
                 "/quantilyxdoc";
    
    // Cache directory
    d->cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    
    // Configuration directory
    d->configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + 
                   "/quantilyxdoc";
    
    // Data directory
    d->dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    
    // Plugins directory
    QString userPluginsDir = d->dataDir + "/plugins";
    QString systemPluginsDir = QCoreApplication::applicationDirPath() + 
                               "/../lib/quantilyxdoc/plugins";
    
    // Prefer user plugins directory, fallback to system
    if (QDir(userPluginsDir).exists()) {
        d->pluginsDir = userPluginsDir;
    } else {
        d->pluginsDir = systemPluginsDir;
    }
    
    // Translations directory
    d->translationsDir = QCoreApplication::applicationDirPath() + 
                         "/../share/quantilyxdoc/translations";
    
    LOG_INFO("Directories initialized:");
    LOG_INFO("  Temp: " << d->tempDir);
    LOG_INFO("  Cache: " << d->cacheDir);
    LOG_INFO("  Config: " << d->configDir);
    LOG_INFO("  Data: " << d->dataDir);
    LOG_INFO("  Plugins: " << d->pluginsDir);
    LOG_INFO("  Translations: " << d->translationsDir);
}

void Application::setupIPC()
{
    // Remove any existing server
    QLocalServer::removeServer(Private::IPC_SERVER_NAME);
    
    // Create local server for IPC
    d->localServer = new QLocalServer(this);
    
    connect(d->localServer, &QLocalServer::newConnection, this, [this]() {
        QLocalSocket* socket = d->localServer->nextPendingConnection();
        
        connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
            QByteArray data = socket->readAll();
            QDataStream stream(&data, QIODevice::ReadOnly);
            QStringList files;
            stream >> files;
            
            LOG_INFO("Received " << files.size() << " files from another instance");
            
            // Open files in main window
            if (d->mainWindow) {
                for (const QString& file : files) {
                    d->mainWindow->openDocument(file);
                }
            }
            
            // Bring window to front
            if (d->mainWindow) {
                d->mainWindow->raise();
                d->mainWindow->activateWindow();
            }
            
            socket->disconnectFromServer();
        });
    });
    
    if (!d->localServer->listen(Private::IPC_SERVER_NAME)) {
        LOG_WARNING("Failed to start IPC server: " << d->localServer->errorString());
    } else {
        LOG_INFO("IPC server started");
    }
}

} // namespace QuantilyxDoc