/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "AuditTrail.h"
#include "Document.h"
#include "Logger.h"
#include "utils/FileUtils.h" // Assuming this exists for file operations
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QThread> // For current thread ID in logs

namespace QuantilyxDoc {

class AuditTrail::Private {
public:
    Private(AuditTrail* q_ptr)
        : q(q_ptr), file(nullptr), maxFileSizeBytes(10 * 1024 * 1024), // 10 MB default
          enabled(true), nextId(1) {}

    AuditTrail* q;
    mutable QMutex mutex; // Protect access to log file and entries list
    QFile* file;
    QString logFilePath;
    qint64 maxFileSizeBytes;
    bool enabled;
    quint64 nextId;
    QList<AuditEntry> recentEntries; // Keep a short-term in-memory list if needed, or just use file

    // Helper to open the log file
    bool openLogFile() {
        if (!enabled) return false;

        if (file && file->isOpen()) {
            return true; // Already open
        }

        if (logFilePath.isEmpty()) {
            // Generate default path
            QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir dir(configDir);
            if (!dir.exists()) dir.mkpath(".");
            logFilePath = dir.filePath("audit_trail.log");
        }

        file = new QFile(logFilePath);
        bool opened = file->open(QIODevice::Append | QIODevice::Text);
        if (!opened) {
            LOG_ERROR("Failed to open audit log file: " << file->errorString());
            delete file;
            file = nullptr;
            return false;
        }

        // Optionally, write a header or separator if file was just created/reopened
        if (file->size() == 0) {
            QTextStream stream(file);
            stream << "# QuantilyxDoc Audit Trail\n";
            stream << "# Format: ID|Timestamp|Type|User|Document|Action|Details|IP|Result|SessionID\n";
            stream.flush();
        }

        LOG_INFO("Opened audit log file: " << logFilePath);
        return true;
    }

    // Helper to write an entry to the file
    bool writeEntryToFile(const AuditEntry& entry) {
        if (!file || !file->isOpen()) {
            if (!openLogFile()) return false;
        }

        // Check file size and rotate if necessary
        if (file->size() >= maxFileSizeBytes) {
            rotateLog();
            if (!openLogFile()) return false; // Re-open new file
        }

        QTextStream stream(file);
        // Format: ID|Timestamp|Type|User|Document|Action|Details|IP|Result|SessionID
        // Use a delimiter unlikely to appear in data, or escape/quote fields properly.
        // For simplicity here, we'll use a pipe, assuming user/document names don't contain it often.
        stream << entry.id << "|"
               << entry.timestamp.toString(Qt::ISODateWithMs) << "|"
               << static_cast<int>(entry.type) << "|"
               << entry.user << "|"
               << entry.documentPath << "|"
               << entry.action << "|"
               << entry.details << "|"
               << entry.ipAddress.toString() << "|"
               << entry.result << "|"
               << entry.sessionId << "\n";
        stream.flush(); // Ensure data is written

        // Optionally, keep a small in-memory cache
        // recentEntries.append(entry);
        // if (recentEntries.size() > 100) recentEntries.removeFirst(); // Keep last 100

        return true;
    }

    // Helper to rotate the log file
    void rotateLog() {
        if (!file || !file->isOpen()) return;

        file->close();
        QString oldPath = logFilePath;
        QString newPath = logFilePath + "." + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".old";
        QFile::rename(oldPath, newPath); // Rename current to timestamped old file
        LOG_INFO("Rotated audit log: " << oldPath << " -> " << newPath);
        emit q->logRotated();
    }

    // Helper to convert EventType enum to string for logging/reading
    QString eventTypeToString(EventType type) {
        switch (type) {
            case EventType::DocumentOpen: return "DOC_OPEN";
            case EventType::DocumentSave: return "DOC_SAVE";
            case EventType::DocumentClose: return "DOC_CLOSE";
            case EventType::DocumentEdit: return "DOC_EDIT";
            case EventType::DocumentPrint: return "DOC_PRINT";
            case EventType::DocumentExport: return "DOC_EXPORT";
            case EventType::UserLogin: return "USER_LOGIN";
            case EventType::UserLogout: return "USER_LOGOUT";
            case EventType::SecurityEvent: return "SECURITY";
            case EventType::SystemEvent: return "SYSTEM";
            default: return "UNKNOWN";
        }
    }
};

// Static instance pointer
AuditTrail* AuditTrail::s_instance = nullptr;

AuditTrail& AuditTrail::instance()
{
    if (!s_instance) {
        s_instance = new AuditTrail();
    }
    return *s_instance;
}

AuditTrail::AuditTrail(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->logFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/audit_trail.log";
    // Ensure directory exists
    QFileInfo info(d->logFilePath);
    QDir dir(info.absolutePath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

AuditTrail::~AuditTrail()
{
    if (d->file && d->file->isOpen()) {
        d->file->close();
    }
    delete d->file;
}

bool AuditTrail::logEvent(const AuditEntry& entry)
{
    if (!d->enabled) return true; // Silently succeed if disabled

    QMutexLocker locker(&d->mutex);
    AuditEntry mutableEntry = entry; // Copy to set ID
    mutableEntry.id = d->nextId++;
    mutableEntry.timestamp = QDateTime::currentDateTime();

    bool success = d->writeEntryToFile(mutableEntry);
    if (success) {
        emit eventLogged(mutableEntry);
    } else {
        LOG_ERROR("Failed to log audit event: " << mutableEntry.action);
    }
    return success;
}

bool AuditTrail::logEvent(AuditEntry::EventType type, const QString& user, Document* document,
                          const QString& action, const QString& details,
                          const QString& result, const QVariantMap& extraData)
{
    if (!d->enabled) return true;

    AuditEntry entry;
    entry.type = type;
    entry.user = user;
    entry.documentPath = document ? document->filePath() : QString();
    entry.action = action;
    entry.details = details;
    entry.result = result;
    entry.extraData = extraData;
    // IP address, session ID would need to be determined by caller or context
    // For local app, IP might be 127.0.0.1 or empty
    // Session ID needs to be managed by an auth/session system

    return logEvent(entry);
}

QList<AuditEntry> AuditTrail::getEntries(const QDateTime& startTime, const QDateTime& endTime,
                                         const QString& userFilter, const QString& docPathFilter,
                                         AuditEntry::EventType typeFilter, int limit) const
{
    QMutexLocker locker(&d->mutex);
    QList<AuditEntry> results;

    // This is a simplified implementation that reads the log file.
    // For performance with large logs, an index or database would be better.
    QFile readFile(d->logFilePath);
    if (!readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open audit log for reading: " << readFile.errorString());
        return results;
    }

    QTextStream stream(&readFile);
    QString line;
    while (readFile.canReadLine()) {
        line = readFile.readLine();
        if (line.startsWith('#')) continue; // Skip header/comment lines

        QStringList parts = line.split('|');
        if (parts.size() < 10) continue; // Malformed line

        bool ok;
        AuditEntry entry;
        entry.id = parts[0].toULongLong(&ok);
        if (!ok) continue;
        entry.timestamp = QDateTime::fromString(parts[1], Qt::ISODateWithMs);
        if (!entry.timestamp.isValid()) continue;
        entry.type = static_cast<AuditEntry::EventType>(parts[2].toInt(&ok));
        if (!ok) continue;
        entry.user = parts[3];
        entry.documentPath = parts[4];
        entry.action = parts[5];
        entry.details = parts[6];
        entry.ipAddress = QHostAddress(parts[7]);
        entry.result = parts[8];
        entry.sessionId = parts[9];

        // Apply filters
        if (startTime.isValid() && entry.timestamp < startTime) continue;
        if (endTime.isValid() && entry.timestamp > endTime) continue;
        if (!userFilter.isEmpty() && entry.user != userFilter) continue;
        if (!docPathFilter.isEmpty() && entry.documentPath != docPathFilter) continue;
        if (typeFilter != AuditEntry::EventType::Unknown && entry.type != typeFilter) continue;

        results.append(entry);

        if (limit > 0 && results.size() >= limit) break;
    }

    // Sort by timestamp descending by default, or as requested
    std::sort(results.begin(), results.end(), [](const AuditEntry& a, const AuditEntry& b) {
        return a.timestamp > b.timestamp;
    });

    return results;
}

quint64 AuditTrail::entryCount() const
{
    // This requires counting lines in the file or maintaining a counter.
    // Counting lines is slow for large files.
    // A better approach is to maintain a counter in memory and persist it.
    // For now, we'll count lines.
    QMutexLocker locker(&d->mutex);
    QFile countFile(d->logFilePath);
    if (!countFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    quint64 count = 0;
    while (countFile.readLine().size() > 0) {
        if (!countFile.atEnd()) count++; // Read one past the last line
    }
    if (!countFile.readLine().isEmpty()) count++; // Count the last line if it doesn't end with \n
    return count;
}

QString AuditTrail::logFilePath() const
{
    QMutexLocker locker(&d->mutex);
    return d->logFilePath;
}

void AuditTrail::setLogFilePath(const QString& path)
{
    QMutexLocker locker(&d->mutex);
    if (d->logFilePath != path) {
        if (d->file && d->file->isOpen()) {
            d->file->close();
        }
        d->logFilePath = path;
        // Directory check is done on write
        LOG_INFO("Audit log file path changed to: " << path);
    }
}

qint64 AuditTrail::maxLogFileSizeBytes() const
{
    QMutexLocker locker(&d->mutex);
    return d->maxFileSizeBytes;
}

void AuditTrail::setMaxLogFileSizeBytes(qint64 size)
{
    if (size <= 0) return;
    QMutexLocker locker(&d->mutex);
    if (d->maxFileSizeBytes != size) {
        d->maxFileSizeBytes = size;
        LOG_INFO("Audit log max file size changed to " << size << " bytes.");
    }
}

bool AuditTrail::isEnabled() const
{
    QMutexLocker locker(&d->mutex);
    return d->enabled;
}

void AuditTrail::setEnabled(bool enabled)
{
    QMutexLocker locker(&d->mutex);
    if (d->enabled != enabled) {
        d->enabled = enabled;
        LOG_INFO("Audit trail logging is " << (enabled ? "enabled" : "disabled"));
    }
}

void AuditTrail::purgeOldEntries()
{
    // This is a stub. Purging based on time/size usually means log rotation.
    // The current rotation happens implicitly when the file gets too big.
    // Purging old *rotated* files would require managing multiple log files.
    LOG_WARN("purgeOldEntries: Currently implemented as log rotation on size. Purging old rotated files not implemented.");
    // To implement purging old files:
    // QDir logDir(QFileInfo(d->logFilePath).absolutePath());
    // QStringList oldLogs = logDir.entryList({"audit_trail.log.*.old"}, QDir::Files, QDir::Time);
    // Remove old files based on age/number/total size.
}

bool AuditTrail::exportEntries(const QString& filePath, const QDateTime& startTime,
                               const QString& userFilter, const QString& docPathFilter,
                               AuditEntry::EventType typeFilter)
{
    QMutexLocker locker(&d->mutex);
    QList<AuditEntry> entries = getEntries(startTime, QDateTime(), userFilter, docPathFilter, typeFilter, 0);

    QFile exportFile(filePath);
    if (!exportFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR("Failed to open export file: " << exportFile.errorString());
        return false;
    }

    QTextStream stream(&exportFile);
    stream << "ID,Timestamp,Type,User,Document,Action,Details,IP,Result,SessionID\n";
    for (const auto& entry : entries) {
        stream << entry.id << ","
               << entry.timestamp.toString(Qt::ISODateWithMs) << ","
               << d->eventTypeToString(entry.type) << ","
               << entry.user << ","
               << entry.documentPath << ","
               << entry.action << ","
               << entry.details << ","
               << entry.ipAddress.toString() << ","
               << entry.result << ","
               << entry.sessionId << "\n";
    }
    stream.flush();
    LOG_INFO("Exported " << entries.size() << " audit entries to: " << filePath);
    return true;
}

bool AuditTrail::verifyIntegrity() const
{
    // This is a basic check. A full implementation would require:
    // 1. Writing a hash/checksum for each entry or blocks of entries.
    // 2. Storing the hash securely (e.g., signed file, separate file).
    // 3. Re-calculating and comparing hashes on verification.
    LOG_WARN("verifyIntegrity: Basic implementation only. Full tamper-proofing requires cryptographic checksums per entry/block.");
    QFile verifyFile(d->logFilePath);
    return verifyFile.exists() && verifyFile.size() > 0;
}

bool AuditTrail::signLog() const
{
    // Requires cryptographic libraries (e.g., OpenSSL via QtNetwork or separate integration).
    // This is a complex feature for a later phase.
    LOG_WARN("signLog: Not implemented. Requires cryptographic library integration.");
    return false;
}

} // namespace QuantilyxDoc