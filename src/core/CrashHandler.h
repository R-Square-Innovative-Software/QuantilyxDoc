/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_CRASHHANDLER_H
#define QUANTILYX_CRASHHANDLER_H

#include <QObject>
#include <QDir>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

/**
 * @brief Manages application crash detection, dump generation, and reporting.
 * 
 * Sets up signal handlers (on Unix) or exception filters (on Windows) to
 * catch crashes, generates minidump files, and provides mechanisms for
 * user reporting.
 */
class CrashHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Information about a detected crash.
     */
    struct CrashInfo {
        QString dumpFilePath;       // Path to the generated dump file
        QString crashReason;        // e.g., "SIGSEGV", "Access Violation"
        QString timestamp;          // ISO date string
        QString applicationVersion; // Version of QuantilyxDoc
        QString operatingSystem;    // OS info
        QString signalOrException;  // Raw signal number or exception code
        QString stackTrace;         // Optional, if retrievable
    };

    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit CrashHandler(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~CrashHandler() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global CrashHandler instance.
     */
    static CrashHandler& instance();

    /**
     * @brief Install the crash handler.
     * This should typically be called early in the application lifecycle.
     * @return True if installation was successful.
     */
    bool install();

    /**
     * @brief Uninstall the crash handler.
     * @return True if uninstallation was successful.
     */
    bool uninstall();

    /**
     * @brief Get the directory where crash dumps are stored.
     * @return Dump directory path.
     */
    QDir dumpDirectory() const;

    /**
     * @brief Set the directory where crash dumps should be stored.
     * @param dir Dump directory path.
     */
    void setDumpDirectory(const QDir& dir);

    /**
     * @brief Check if crash reporting is enabled.
     * @return True if enabled.
     */
    bool isEnabled() const;

    /**
     * @brief Enable or disable crash reporting.
     * @param enabled Whether to enable crash reporting.
     */
    void setEnabled(bool enabled);

    /**
     * @brief Simulate a crash for testing purposes.
     * This function will intentionally cause the application to crash.
     */
    void simulateCrash();

    /**
     * @brief Get the path to the crash reporting executable (if separate).
     * @return Path to the reporter executable.
     */
    QString reporterExecutablePath() const;

    /**
     * @brief Set the path to the crash reporting executable.
     * @param path Path to the reporter executable.
     */
    void setReporterExecutablePath(const QString& path);

    /**
     * @brief Get the number of crash dumps found in the dump directory.
     * @return Count of dump files.
     */
    int pendingDumpCount() const;

    /**
     * @brief Get a list of paths to pending crash dumps.
     * @return List of dump file paths.
     */
    QStringList pendingDumpPaths() const;

    /**
     * @brief Submit a specific dump file for analysis/reporting.
     * @param dumpFilePath Path to the dump file to submit.
     * @return True if submission was initiated successfully.
     */
    bool submitDump(const QString& dumpFilePath);

    /**
     * @brief Submit all pending dump files for analysis/reporting.
     * @return True if submission was initiated successfully.
     */
    bool submitAllPendingDumps();

    /**
     * @brief Clear all pending dump files from the dump directory.
     * @return True if clearing was successful.
     */
    bool clearAllDumps();

    /**
     * @brief Get the last recorded crash information (if any recent crash).
     * @return CrashInfo struct, or invalid one if none.
     */
    CrashInfo lastCrashInfo() const;

signals:
    /**
     * @brief Emitted when a crash is detected and a dump is generated.
     * @param info Information about the crash.
     */
    void crashDetected(const QuantilyxDoc::CrashHandler::CrashInfo& info);

    /**
     * @brief Emitted when a dump file is successfully submitted.
     * @param dumpFilePath Path to the submitted dump.
     */
    void dumpSubmitted(const QString& dumpFilePath);

    /**
     * @brief Emitted when dump submission fails.
     * @param dumpFilePath Path to the dump that failed.
     * @param error Error message.
     */
    void dumpSubmissionFailed(const QString& dumpFilePath, const QString& error);

    /**
     * @brief Emitted when pending dumps are cleared.
     */
    void dumpsCleared();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_CRASHHANDLER_H