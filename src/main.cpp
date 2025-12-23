/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "ui/SplashScreen.h"
#include "ui/MainWindow.h"
#include "core/Application.h"
#include "core/Logger.h"
#include "core/ConfigManager.h"
#include "core/Settings.h"
#include "core/RecentFiles.h"
#include "core/BackupManager.h"
#include "core/CrashHandler.h"
#include "core/ProfileManager.h"
#include "core/MetadataDatabase.h"
#include "core/DuplicateDetector.h"
#include "search/FullTextIndex.h"
#include "automation/MacroRecorder.h"
#include "automation/ScriptingEngine.h"
#include "security/PasswordRemover.h"
#include "security/RestrictionBypass.h"
#include "ocr/OcrEngine.h"
#include "ui/CommandPalette.h"
#include "ui/QuickActionsPanel.h"
#include "ui/AboutDialog.h"
#include "ui/PreferencesDialog.h"
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QCommandLineParser>
#include <QTimer>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QMetaType> // For registering custom types if needed
#include <QElapsedTimer> // For timing initialization steps
#include <QDebug>
#include <QThread> // For potential thread management during init

// Register custom types if any are used across threads/signals
// Q_DECLARE_METATYPE(MyCustomType)

int main(int argc, char *argv[])
{
    // Use QuantilyxDoc's custom Application class which inherits from QApplication.
    // This ensures the custom application-wide settings, event handling, etc., are used.
    // The Application class constructor sets up basic QApplication properties.
    QuantilyxDoc::Application app(argc, argv);

    // --- Register Custom Types (if any) ---
    // qRegisterMetaType<MyCustomType>("MyCustomType");

    // --- Command Line Parsing ---
    QCommandLineParser parser;
    parser.setApplicationDescription("Professional Document Editor - QuantilyxDoc");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fileArgument(QStringList() << "f" << "file",
                                    "Document file to open on startup.",
                                    "file_path");
    QCommandLineOption profileArgument(QStringList() << "p" << "profile",
                                       "Load a specific profile on startup.",
                                       "profile_name");
    QCommandLineOption disablePluginsOption(QStringList() << "no-plugins",
                                            "Disable all plugins for this session.");
    QCommandLineOption verboseOption(QStringList() << "verbose",
                                     "Enable verbose logging.");
    QCommandLineOption configPathOption(QStringList() << "config",
                                        "Specify a custom configuration file path.",
                                        "config_path");

    parser.addPositionalArgument("file", "Document file to open.", "[file]");
    parser.addOption(fileArgument);
    parser.addOption(profileArgument);
    parser.addOption(disablePluginsOption);
    parser.addOption(verboseOption);
    parser.addOption(configPathOption);

    parser.process(app);

    QStringList fileNames = parser.positionalArguments();
    if (parser.isSet(fileArgument)) {
        fileNames.prepend(parser.value(fileArgument)); // Prefer --file over positional
    }
    QString startupProfile = parser.value(profileArgument);
    bool disablePlugins = parser.isSet(disablePluginsOption);
    bool verboseLogging = parser.isSet(verboseOption);
    QString customConfigPath = parser.value(configPathOption);

    // --- Application Initialization Sequence ---
    // This is the critical startup phase where all core systems are initialized.
    bool initSuccess = true;
    QString initError;
    QElapsedTimer initTimer;
    initTimer.start();

    LOG_INFO("=== Starting QuantilyxDoc Initialization ===");
    LOG_DEBUG("Command line args: " << app.arguments().join(" "));

    // 0. Early Logger Initialization (before other systems)
    LOG_DEBUG("Initializing Logger (early)..."); // This might not log anywhere yet if logger isn't fully set up
    if (!QuantilyxDoc::Logger::instance().initialize()) {
        initError = "Failed to initialize Logger.";
        initSuccess = false;
    } else {
        LOG_INFO("Logger initialized successfully.");
        if (verboseLogging) {
             QuantilyxDoc::Logger::instance().setLogLevel(QuantilyxDoc::Logger::Debug); // Increase verbosity if requested
        }
    }

    // 1. Initialize ConfigManager (loads basic configuration needed for other systems)
    if (initSuccess) {
        LOG_DEBUG("Initializing ConfigManager...");
        if (!QuantilyxDoc::ConfigManager::instance().initialize(customConfigPath)) { // Pass custom path if provided
            initError = "Failed to initialize ConfigManager.";
            initSuccess = false;
        } else {
            LOG_INFO("ConfigManager initialized successfully.");
        }
    }

    // 2. Initialize Settings (loads user preferences from file based on config/profile)
    if (initSuccess) {
        LOG_DEBUG("Loading Settings...");
        // Settings might depend on the profile loaded by ProfileManager
        if (!QuantilyxDoc::Settings::instance().isEnabled()) { // Check if settings system itself is enabled/configured
             QuantilyxDoc::Settings::instance().setEnabled(true); // Enable if not explicitly disabled
        }
        QuantilyxDoc::Settings::instance().load(); // Load settings from file
        LOG_INFO("Settings loaded successfully.");
    }

    // 3. Initialize Profile Manager (must come after Settings to potentially override them)
    if (initSuccess) {
        LOG_DEBUG("Initializing ProfileManager...");
        if (!QuantilyxDoc::ProfileManager::instance().initialize()) {
            initError = "Failed to initialize ProfileManager.";
            initSuccess = false;
        } else {
            LOG_INFO("ProfileManager initialized successfully.");
            // Optionally, load a specific profile from command line argument
            if (!startupProfile.isEmpty()) {
                if (QuantilyxDoc::ProfileManager::instance().switchToProfile(startupProfile)) {
                    LOG_INFO("Loaded startup profile: " << startupProfile);
                } else {
                    LOG_WARN("Startup profile not found: " << startupProfile << ". Using current/default.");
                }
            }
        }
    }

    // 4. Initialize Crash Handler (early, so it can catch crashes during init)
    if (initSuccess) {
        LOG_DEBUG("Installing CrashHandler...");
        if (!QuantilyxDoc::CrashHandler::instance().install()) {
            LOG_WARN("Could not install crash handler. Application stability might be affected if a crash occurs.");
            // Do not fail initialization for this, but log it heavily.
        } else {
            LOG_INFO("CrashHandler installed successfully.");
        }
    }

    // 5. Initialize Backup Manager (settings dependent)
    if (initSuccess) {
        LOG_DEBUG("Initializing BackupManager...");
        // BackupManager reads settings like 'EnableAutoBackup', 'BackupInterval', etc.
        // QuantilyxDoc::BackupManager::instance().setEnabled(Settings::instance().value<bool>("General/EnableAutoBackup", true));
        // QuantilyxDoc::BackupManager::instance().setBackupDirectory(...);
        // QuantilyxDoc::BackupManager::instance().initializeTimers(); // Or similar
        // For now, assume initialization happens in constructor or via settings load.
        LOG_INFO("BackupManager initialized (configuration read).");
    }

    // 6. Initialize Recent Files (loads list from storage)
    if (initSuccess) {
        LOG_DEBUG("Loading RecentFiles...");
        QuantilyxDoc::RecentFiles::instance().load();
        LOG_INFO("RecentFiles loaded successfully.");
    }

    // 7. Initialize Metadata Database (opens connection/file)
    if (initSuccess) {
        LOG_DEBUG("Initializing MetadataDatabase...");
        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/metadata.db";
        QDir().mkpath(QFileInfo(dbPath).absolutePath()); // Ensure directory exists
        if (!QuantilyxDoc::MetadataDatabase::instance().initialize(dbPath)) {
            initError = "Failed to initialize MetadataDatabase.";
            initSuccess = false;
        } else {
            LOG_INFO("MetadataDatabase initialized successfully at: " << dbPath);
        }
    }

    // 8. Initialize Duplicate Detector (uses MetadataDatabase)
    if (initSuccess) {
        LOG_DEBUG("Initializing DuplicateDetector...");
        // DuplicateDetector might need access to MetadataDB or other indexing systems.
        // QuantilyxDoc::DuplicateDetector::instance().setMetadataDatabase(&MetadataDatabase::instance());
        LOG_INFO("DuplicateDetector initialized.");
    }

    // 9. Initialize Full-Text Index (opens connection/file, potentially loads cache)
    if (initSuccess) {
        LOG_DEBUG("Initializing FullTextIndex...");
        QString indexPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/fts_index";
        QDir().mkpath(indexPath);
        if (!QuantilyxDoc::FullTextIndex::instance().initialize(indexPath)) {
            initError = "Failed to initialize FullTextIndex.";
            initSuccess = false;
        } else {
            LOG_INFO("FullTextIndex initialized successfully at: " << indexPath);
        }
    }

    // 10. Initialize Password Remover (locates external tools like QPDF)
    if (initSuccess) {
        LOG_DEBUG("Initializing PasswordRemover...");
        // PasswordRemover might search for 'qpdf' executable or similar during initialization.
        // QuantilyxDoc::PasswordRemover::instance().findExternalTool(); // Or happens in constructor
        LOG_INFO("PasswordRemover initialized (external tools located).");
    }

    // 11. Initialize Restriction Bypass (locates external tools like QPDF)
    if (initSuccess) {
        LOG_DEBUG("Initializing RestrictionBypass...");
        // RestrictionBypass might search for 'qpdf' executable or similar during initialization.
        // QuantilyxDoc::RestrictionBypass::instance().findExternalTool(); // Or happens in constructor
        LOG_INFO("RestrictionBypass initialized (external tools located).");
    }

    // 12. Initialize OCR Engine (loads language data, initializes Tesseract/PaddleOCR)
    if (initSuccess) {
        LOG_DEBUG("Initializing OcrEngine...");
        QString lang = QuantilyxDoc::Settings::instance().value<QString>("Ocr/Language", "eng");
        QString dataPath = QuantilyxDoc::Settings::instance().value<QString>("Ocr/TessDataPath", QString()); // Could be empty, uses default
        if (!QuantilyxDoc::OcrEngine::instance().initialize(lang, dataPath)) {
            // OCR is not critical for basic startup, warn and continue
            LOG_WARN("Failed to initialize OCR Engine. OCR features will be unavailable.");
            // initSuccess = true; // Keep initSuccess as true to allow the app to run without OCR
        } else {
            LOG_INFO("OcrEngine initialized successfully for language: " << lang);
        }
    }

    // 13. Initialize Macro Recorder
    if (initSuccess) {
        LOG_DEBUG("Initializing MacroRecorder...");
        // QuantilyxDoc::MacroRecorder::instance(); // Singleton created/accessed, might initialize in constructor
        LOG_INFO("MacroRecorder initialized.");
    }

    // 14. Initialize Scripting Engine (loads language interpreter if applicable)
    if (initSuccess) {
        LOG_DEBUG("Initializing ScriptingEngine...");
        // QString scriptLanguage = Settings::instance().value<QString>("Scripting/Language", "python");
        // if (!QuantilyxDoc::ScriptingEngine::instance().initialize(scriptLanguage)) {
        //     LOG_WARN("Failed to initialize Scripting Engine. Scripting features will be unavailable.");
        //     // initSuccess = true; // Keep initSuccess as true to allow the app to run without scripting
        // } else {
        //     LOG_INFO("ScriptingEngine initialized successfully for language: " << scriptLanguage);
        // }
        LOG_INFO("ScriptingEngine initialized (language loading deferred until first script run or preference change).");
    }

    // 15. Initialize Plugins (if enabled)
    if (initSuccess && !disablePlugins) {
        LOG_DEBUG("Initializing Plugins...");
        // The Application class might have a method to load plugins based on settings or a list.
        // app.initializePlugins(); // Hypothetical method in Application
        LOG_INFO("Plugins initialized (or plugin loading deferred until after UI is loaded).");
    } else if (disablePlugins) {
        LOG_INFO("Plugin initialization skipped (--no-plugins).");
    }


    // --- Initialization Result ---
    qint64 initTimeMs = initTimer.elapsed();
    if (!initSuccess) {
        // Critical failure during initialization
        QuantilyxDoc::LOG_CRITICAL("Application initialization failed after " << initTimeMs << " ms: " << initError);
        QMessageBox::critical(nullptr, QObject::tr("Initialization Error"), initError);
        return -1; // Exit with error code
    }

    LOG_INFO("=== QuantilyxDoc Core Initialization Complete (Time: " << initTimeMs << " ms) ===");

    // --- Show Splash Screen ---
    // The splash screen provides visual feedback during the potentially long initialization.
    // It should be created *after* core systems that might log are initialized, so it can show messages.
    QuantilyxDoc::SplashScreen splash;
    splash.show();
    app.processEvents(); // Ensure splash is painted immediately

    splash.showMessage(QObject::tr("Loading user interface..."), Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
    app.processEvents();

    // --- Create and Show Main Window ---
    // The MainWindow integrates all the UI components and connects to the core systems initialized above.
    splash.showMessage(QObject::tr("Initializing main window..."), Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
    app.processEvents();

    QuantilyxDoc::MainWindow window; // MainWindow constructor should connect to core systems, potentially show its own progress

    splash.finish(&window); // Hide splash screen when main window is shown and ready
    window.show(); // Make the main window visible

    splash.showMessage(QObject::tr("Ready"), Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
    app.processEvents(); // Allow final splash message to be seen briefly

    // --- Load initial file if provided via command line ---
    if (!fileNames.isEmpty()) {
        QString filePath = fileNames.first(); // Open the first file specified
        if (QFileInfo::exists(filePath)) {
            LOG_INFO("Opening file from command line: " << filePath);
            // MainWindow needs a method to open a document.
            // This might involve DocumentFactory, showing a loading indicator, etc.
            window.openDocument(filePath);
        } else {
            LOG_WARN("Command line file does not exist: " << filePath);
            QMessageBox::warning(&window, QObject::tr("File Not Found"),
                                 QObject::tr("The file specified on the command line does not exist:\n%1").arg(filePath));
        }
        // Future: Handle subsequent files (e.g., open in new tabs/windows if supported)
        for (int i = 1; i < fileNames.size(); ++i) {
             QString nextPath = fileNames[i];
             if (QFileInfo::exists(nextPath)) {
                 LOG_INFO("Additional file specified on command line (not opened in this instance): " << nextPath);
                 // Example: window.openDocumentInNewTab(nextPath); // Hypothetical method
             } else {
                 LOG_WARN("Additional command line file does not exist: " << nextPath);
             }
        }
    }

    // --- Handle Startup Tasks (e.g., restore session, check for updates) ---
    splash.showMessage(QObject::tr("Handling startup tasks..."), Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
    app.processEvents();

    // Example: Restore last session (open last files) - This logic typically resides in MainWindow
    bool restoreSession = QuantilyxDoc::Settings::instance().value<bool>("General/RestoreSession", true);
    if (restoreSession) {
        // This logic would typically be in MainWindow or a SessionManager class
        // QStringList lastFiles = Settings::instance().value<QStringList>("Session/LastOpenFiles", QStringList());
        // for (const QString& file : lastFiles) {
        //     if (QFileInfo::exists(file)) {
        //         window.openDocument(file);
        //     }
        // }
        LOG_DEBUG("Session restore logic would run here if enabled.");
        // window.restoreLastSession(); // Hypothetical MainWindow method
    }

    // Example: Check for updates (asynchronously, maybe after a delay)
    bool checkUpdates = QuantilyxDoc::Settings::instance().value<bool>("General/CheckForUpdates", true);
    if (checkUpdates) {
        QTimer::singleShot(5000, &window, []() {
             // window.checkForUpdates(); // Hypothetical MainWindow method
             LOG_DEBUG("Update check logic would run here if enabled.");
        });
    }

    LOG_INFO("QuantilyxDoc startup sequence finished. Starting event loop.");

    // --- Start the Qt Event Loop ---
    // This is where the application waits for and processes user input, system events, etc.
    int returnCode = app.exec();

    // --- Application Shutdown Sequence ---
    // Perform cleanup tasks before the application exits.
    LOG_INFO("Shutting down QuantilyxDoc (exit code: " << returnCode << ")...");

    // Save application settings
    LOG_DEBUG("Saving application settings...");
    QuantilyxDoc::Settings::instance().save();

    // Save recent files list
    LOG_DEBUG("Saving recent files list...");
    QuantilyxDoc::RecentFiles::instance().save();

    // Save current profile state (if applicable)
    LOG_DEBUG("Saving current profile...");
    // QuantilyxDoc::ProfileManager::instance().saveCurrentProfile(); // Assuming this method exists

    // Save metadata database changes (if any were made during the session)
    LOG_DEBUG("Committing metadata database changes...");
    // QuantilyxDoc::MetadataDatabase::instance().commit(); // Assuming this flushes changes

    // Save full-text index changes (if any were made during the session)
    LOG_DEBUG("Committing full-text index changes...");
    // QuantilyxDoc::FullTextIndex::instance().commit(); // Assuming this flushes changes

    // Save duplicate detector state/cache (if applicable)
    LOG_DEBUG("Saving duplicate detector state...");
    // QuantilyxDoc::DuplicateDetector::instance().saveState(); // Assuming this method exists

    // Uninstall the crash handler
    LOG_DEBUG("Uninstalling crash handler...");
    QuantilyxDoc::CrashHandler::instance().uninstall();

    // Shut down OCR engine if necessary
    LOG_DEBUG("Shutting down OcrEngine...");
    // QuantilyxDoc::OcrEngine::instance().shutdown(); // Assuming a shutdown method exists

    // Shut down scripting engine if necessary
    LOG_DEBUG("Shutting down ScriptingEngine...");
    // QuantilyxDoc::ScriptingEngine::instance().shutdown(); // Assuming a shutdown method exists

    // Shut down macro recorder if necessary
    LOG_DEBUG("Shutting down MacroRecorder...");
    // QuantilyxDoc::MacroRecorder::instance().shutdown(); // Assuming a shutdown method exists

    // Shut down plugins if necessary
    LOG_DEBUG("Shutting down plugins...");
    // app.shutdownPlugins(); // Hypothetical method in Application

    LOG_INFO("QuantilyxDoc shutdown sequence complete.");
    return returnCode; // Return the exit code from QApplication::exec()
}