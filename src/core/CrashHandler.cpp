/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "CrashHandler.h"
#include "Logger.h"
#include "utils/FileUtils.h" // Assuming this exists
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>

// Platform-specific includes for crash handling
#ifdef Q_OS_WIN
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC) // Linux, BSD, etc.
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/resource.h> // For core dump size limits
#endif

namespace QuantilyxDoc {

class CrashHandler::Private {
public:
    Private(CrashHandler* q_ptr)
        : q(q_ptr), installed(false), enabled(true), dumpDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crash_dumps") {}

    CrashHandler* q;
    mutable QMutex mutex; // Protect state changes
    bool installed;
    bool enabled;
    QDir dumpDir;
    QString reporterPath;
    CrashInfo lastCrash;

    // Platform-specific handler setup/teardown
#ifdef Q_OS_WIN
    static LONG WINAPI winExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo);
    static bool setupWindowsHandler();
    static bool teardownWindowsHandler();
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    static void unixSignalHandler(int sig);
    static bool setupUnixHandler();
    static bool teardownUnixHandler();
#endif

    // Helper to create dump directory
    bool ensureDumpDirExists() {
        if (!dumpDir.exists()) {
            return dumpDir.mkpath(".");
        }
        return true;
    }

    // Helper to get OS info string
    QString getOSInfo() const {
#ifdef Q_OS_WIN
        return "Windows";
#elif defined(Q_OS_MAC)
        return "macOS";
#elif defined(Q_OS_LINUX)
        return "Linux";
#else
        return "Unknown Unix";
#endif
    }
};

// Static instance pointer
CrashHandler* CrashHandler::s_instance = nullptr;

CrashHandler& CrashHandler::instance()
{
    if (!s_instance) {
        s_instance = new CrashHandler();
    }
    return *s_instance;
}

CrashHandler::CrashHandler(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    // Ensure dump directory exists on construction
    d->ensureDumpDirExists();
    LOG_INFO("CrashHandler initialized. Dump directory: " << d->dumpDir.absolutePath());
}

CrashHandler::~CrashHandler()
{
    uninstall(); // Ensure handler is uninstalled on destruction
}

bool CrashHandler::install()
{
    QMutexLocker locker(&d->mutex);
    if (d->installed || !d->enabled) return false;

    bool success = false;
#ifdef Q_OS_WIN
    success = d->setupWindowsHandler();
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    success = d->setupUnixHandler();
#endif
    if (success) {
        d->installed = true;
        LOG_INFO("Crash handler installed.");
    } else {
        LOG_ERROR("Failed to install crash handler.");
    }
    return success;
}

bool CrashHandler::uninstall()
{
    QMutexLocker locker(&d->mutex);
    if (!d->installed) return true;

    bool success = true;
#ifdef Q_OS_WIN
    success = d->teardownWindowsHandler();
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    success = d->teardownUnixHandler();
#endif
    if (success) {
        d->installed = false;
        LOG_INFO("Crash handler uninstalled.");
    } else {
        LOG_WARN("Failed to uninstall crash handler.");
    }
    return success;
}

QDir CrashHandler::dumpDirectory() const
{
    QMutexLocker locker(&d->mutex);
    return d->dumpDir;
}

void CrashHandler::setDumpDirectory(const QDir& dir)
{
    QMutexLocker locker(&d->mutex);
    if (d->dumpDir != dir) {
        d->dumpDir = dir;
        d->ensureDumpDirExists(); // Attempt to create new directory
        LOG_INFO("Crash dump directory changed to: " << dir.absolutePath());
    }
}

bool CrashHandler::isEnabled() const
{
    QMutexLocker locker(&d->mutex);
    return d->enabled;
}

void CrashHandler::setEnabled(bool enabled)
{
    QMutexLocker locker(&d->mutex);
    if (d->enabled != enabled) {
        d->enabled = enabled;
        if (enabled && !d->installed) {
            install(); // Install if enabled and not already installed
        } else if (!enabled && d->installed) {
            uninstall(); // Uninstall if disabled and installed
        }
        LOG_INFO("Crash handler is now " << (enabled ? "enabled" : "disabled"));
    }
}

void CrashHandler::simulateCrash()
{
    LOG_WARN("Simulating crash...");
    // Voluntary crash - this will trigger the handler if installed
    volatile int* p = nullptr;
    *p = 42; // Dereference null pointer
}

QString CrashHandler::reporterExecutablePath() const
{
    QMutexLocker locker(&d->mutex);
    return d->reporterPath;
}

void CrashHandler::setReporterExecutablePath(const QString& path)
{
    QMutexLocker locker(&d->mutex);
    d->reporterPath = path;
    LOG_INFO("Crash reporter path set to: " << path);
}

int CrashHandler::pendingDumpCount() const
{
    QMutexLocker locker(&d->mutex);
    if (!d->dumpDir.exists()) return 0;
    QStringList filters = {"*.dmp", "*.dump"}; // Common dump file extensions
    return d->dumpDir.entryList(filters, QDir::Files).size();
}

QStringList CrashHandler::pendingDumpPaths() const
{
    QMutexLocker locker(&d->mutex);
    if (!d->dumpDir.exists()) return QStringList();
    QStringList filters = {"*.dmp", "*.dump"};
    QStringList files = d->dumpDir.entryList(filters, QDir::Files);
    QStringList fullPaths;
    fullPaths.reserve(files.size());
    for (const QString& file : files) {
        fullPaths.append(d->dumpDir.filePath(file));
    }
    return fullPaths;
}

bool CrashHandler::submitDump(const QString& dumpFilePath)
{
    // This would typically launch the reporter executable with the dump file as an argument.
    // For a stub, we'll just log.
    Q_UNUSED(dumpFilePath);
    LOG_WARN("submitDump: Stub implementation. Would launch reporter with: " << dumpFilePath);
    // QProcess::startDetached(reporterExecutablePath(), {dumpFilePath});
    emit dumpSubmitted(dumpFilePath);
    return true; // Assume success for stub
}

bool CrashHandler::submitAllPendingDumps()
{
    bool allSuccess = true;
    QStringList dumps = pendingDumpPaths();
    for (const QString& dumpPath : dumps) {
        if (!submitDump(dumpPath)) {
            allSuccess = false;
            emit dumpSubmissionFailed(dumpPath, "Stub implementation failed");
        }
    }
    return allSuccess;
}

bool CrashHandler::clearAllDumps()
{
    QMutexLocker locker(&d->mutex);
    bool success = true;
    QStringList dumps = pendingDumpPaths();
    for (const QString& dumpPath : dumps) {
        QFile dumpFile(dumpPath);
        if (!dumpFile.remove()) {
            LOG_ERROR("Failed to remove crash dump: " << dumpPath << ", Error: " << dumpFile.errorString());
            success = false;
        }
    }
    if (success && !dumps.isEmpty()) {
        LOG_INFO("Cleared " << dumps.size() << " crash dump files.");
        emit dumpsCleared();
    }
    return success;
}

CrashHandler::CrashInfo CrashHandler::lastCrashInfo() const
{
    QMutexLocker locker(&d->mutex);
    return d->lastCrash;
}

// Platform-specific implementations
#ifdef Q_OS_WIN
LONG CrashHandler::Private::winExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    // Get QuantilyxDoc instance to emit signal
    CrashHandler* instance = &CrashHandler::instance();
    Private* d = instance->d.data();

    if (!d->enabled) return EXCEPTION_CONTINUE_SEARCH;

    // Generate dump file
    QString dumpFileName = QString("quantilyxdoc_crash_%1.dmp").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString dumpPath = d->dumpDir.filePath(dumpFileName);

    HANDLE hDumpFile = CreateFile(
        (wchar_t*)dumpPath.utf16(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDumpFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION ExpParam;
        ExpParam.ThreadId = GetCurrentThreadId();
        ExpParam.ExceptionPointers = ExceptionInfo;
        ExpParam.ClientPointers = TRUE;

        BOOL written = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hDumpFile,
            MiniDumpWithDataSegs, // Or a more comprehensive type
            &ExpParam,
            NULL,
            NULL
        );
        CloseHandle(hDumpFile);

        if (written) {
            LOG_ERROR("Crash dump written to: " << dumpPath);

            // Populate CrashInfo
            CrashInfo info;
            info.dumpFilePath = dumpPath;
            info.crashReason = "Windows Exception";
            info.signalOrException = QString::number(ExceptionInfo->ExceptionRecord->ExceptionCode, 16);
            info.timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
            info.applicationVersion = QCoreApplication::applicationVersion();
            info.operatingSystem = d->getOSInfo();
            // Stack trace would require more complex processing with dbghelp

            d->lastCrash = info;
            emit instance->crashDetected(info);

            // Optionally launch reporter here, but it might be risky in a crashed state.
            // It's safer to launch it on next app start if a dump is found.

        } else {
            LOG_ERROR("Failed to write minidump.");
        }
    } else {
        LOG_ERROR("Failed to create dump file: " << dumpPath);
    }

    // Return EXCEPTION_EXECUTE_HANDLER to terminate the process.
    // Returning EXCEPTION_CONTINUE_SEARCH might allow other handlers to run.
    return EXCEPTION_EXECUTE_HANDLER;
}

bool CrashHandler::Private::setupWindowsHandler() {
    SetUnhandledExceptionFilter(winExceptionHandler);
    return true; // Assume success for this stub
}

bool CrashHandler::Private::teardownWindowsHandler() {
    SetUnhandledExceptionFilter(NULL);
    return true;
}

#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
void CrashHandler::Private::unixSignalHandler(int sig) {
    // Get QuantilyxDoc instance to emit signal
    CrashHandler* instance = &CrashHandler::instance();
    Private* d = instance->d.data();

    if (!d->enabled) signal(sig, SIG_DFL); // Revert to default and re-raise

    // Generate dump file - on Unix, this often means ensuring core dumps are allowed/configured
    // and maybe writing a small metadata file here.
    QString dumpFileName = QString("quantilyxdoc_crash_%1.dump").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString dumpPath = d->dumpDir.filePath(dumpFileName);

    // For a full implementation, you'd use backtrace() and backtrace_symbols() to get a stack trace.
    // Writing a core dump from within the signal handler is complex and often not recommended.
    // A common approach is to write a small metadata file and rely on system core dumps or
    // external tools (like Breakpad/Google Crashpad) which are better suited for this.

    // Stub: write a simple metadata file
    QFile metaFile(dumpPath + ".meta");
    if (metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&metaFile);
        stream << "Signal: " << sig << "\n";
        stream << "Timestamp: " << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << "\n";
        stream << "PID: " << QCoreApplication::applicationPid() << "\n";
        stream << "App Version: " << QCoreApplication::applicationVersion() << "\n";
        stream << "OS: " << d->getOSInfo() << "\n";
        metaFile.close();
        LOG_ERROR("Crash metadata written to: " << dumpPath << ".meta");
    }

    // Populate CrashInfo
    CrashInfo info;
    info.dumpFilePath = dumpPath + ".meta"; // Point to our metadata file for now
    info.crashReason = QString("Signal %1").arg(sig);
    info.signalOrException = QString::number(sig);
    info.timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    info.applicationVersion = QCoreApplication::applicationVersion();
    info.operatingSystem = d->getOSInfo();

    d->lastCrash = info;
    emit instance->crashDetected(info);

    // Revert to default handler and re-raise signal to terminate process properly
    signal(sig, SIG_DFL);
    raise(sig);
}

bool CrashHandler::Private::setupUnixHandler() {
    // Install handlers for common crash signals
    signal(SIGSEGV, unixSignalHandler);
    signal(SIGABRT, unixSignalHandler);
    signal(SIGBUS, unixSignalHandler);
    signal(SIGILL, unixSignalHandler);
    signal(SIGFPE, unixSignalHandler);
    // SIGPIPE is often ignored by default, might not cause crash but terminate
    // signal(SIGPIPE, unixSignalHandler); // Usually not a crash, but can terminate
    return true;
}

bool CrashHandler::Private::teardownUnixHandler() {
    // Revert to default handlers
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    return true;
}
#endif // Q_OS_UNIX

} // namespace QuantilyxDoc