/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_AUDITTRAIL_H
#define QUANTILYX_AUDITTRAIL_H

#include <QObject>
#include <QDateTime>
#include <QHostAddress>
#include <QMutex>
#include <memory>
#include <QFile>

namespace QuantilyxDoc {

class Document;

/**
 * @brief Represents a single entry in the audit trail.
 */
struct AuditEntry {
    enum class EventType {
        Unknown,
        DocumentOpen,
        DocumentSave,
        DocumentClose,
        DocumentEdit,
        DocumentPrint,
        DocumentExport,
        UserLogin,
        UserLogout,
        SecurityEvent, // e.g., failed password attempt
        SystemEvent    // e.g., application start/stop
    };

    quint64 id;             // Unique sequential ID
    QDateTime timestamp;    // When the event occurred
    EventType type;         // Type of event
    QString user;           // User associated with the event
    QString documentPath;   // Path of the document involved (if any)
    QString action;         // Specific action taken (e.g., "saved", "edited page 5")
    QString details;        // Additional details about the event
    QHostAddress ipAddress; // IP address (if network event, or client IP)
    QString result;         // Result of the action (e.g., "success", "failure", "error message")
    QString sessionId;      // Session ID (for grouping related events)
    QVariantMap extraData;  // Any other relevant data

    AuditEntry() : id(0), type(EventType::Unknown) {}
};

/**
 * @brief Manages the audit trail for the application and documents.
 * 
 * Logs significant events like document opens, saves, edits, user logins,
 * and security-related actions to a secure, tamper-evident log file.
 * Supports filtering and querying the log.
 */
class AuditTrail : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit AuditTrail(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~AuditTrail() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global AuditTrail instance.
     */
    static AuditTrail& instance();

    /**
     * @brief Log an event to the audit trail.
     * @param entry The audit entry to log.
     * @return True if logging was successful.
     */
    bool logEvent(const AuditEntry& entry);

    /**
     * @brief Log a convenience event with common parameters.
     * @param type Type of the event.
     * @param user User performing the action.
     * @param document Document involved (can be nullptr).
     * @param action Description of the action.
     * @param details Optional details.
     * @param result Optional result string.
     * @param extraData Optional extra data map.
     * @return True if logging was successful.
     */
    bool logEvent(AuditEntry::EventType type, const QString& user, Document* document,
                  const QString& action, const QString& details = QString(),
                  const QString& result = QString(), const QVariantMap& extraData = QVariantMap());

    /**
     * @brief Get audit entries based on filters.
     * @param startTime Earliest time to include.
     * @param endTime Latest time to include.
     * @param userFilter Filter by user name (empty for all).
     * @param docPathFilter Filter by document path (empty for all).
     * @param typeFilter Filter by event type (Unknown for all).
     * @param limit Maximum number of entries to return (0 for all).
     * @return List of matching audit entries.
     */
    QList<AuditEntry> getEntries(const QDateTime& startTime = QDateTime(),
                                 const QDateTime& endTime = QDateTime(),
                                 const QString& userFilter = QString(),
                                 const QString& docPathFilter = QString(),
                                 AuditEntry::EventType typeFilter = AuditEntry::EventType::Unknown,
                                 int limit = 0) const;

    /**
     * @brief Get the total number of entries in the log.
     * @return Entry count.
     */
    quint64 entryCount() const;

    /**
     * @brief Get the path to the audit log file.
     * @return Log file path.
     */
    QString logFilePath() const;

    /**
     * @brief Set the path for the audit log file.
     * @param path Log file path.
     */
    void setLogFilePath(const QString& path);

    /**
     * @brief Get the maximum size of the audit log file in bytes.
     * @return Maximum size in bytes.
     */
    qint64 maxLogFileSizeBytes() const;

    /**
     * @brief Set the maximum size of the audit log file in bytes.
     * @param size Maximum size in bytes.
     */
    void setMaxLogFileSizeBytes(qint64 size);

    /**
     * @brief Check if audit logging is enabled.
     * @return True if enabled.
     */
    bool isEnabled() const;

    /**
     * @brief Enable or disable audit logging.
     * @param enabled Whether to enable logging.
     */
    void setEnabled(bool enabled);

    /**
     * @brief Purge old log entries based on time or size limits.
     */
    void purgeOldEntries();

    /**
     * @brief Export a filtered set of audit entries to a file (e.g., CSV).
     * @param filePath Path to export file.
     * @param startTime Earliest time to include.
     * @param endTime Latest time to include.
     * @param userFilter Filter by user name.
     * @param docPathFilter Filter by document path.
     * @param typeFilter Filter by event type.
     * @return True if export was successful.
     */
    bool exportEntries(const QString& filePath,
                       const QDateTime& startTime = QDateTime(),
                       const QDateTime& endTime = QDateTime(),
                       const QString& userFilter = QString(),
                       const QString& docPathFilter = QString(),
                       AuditEntry::EventType typeFilter = AuditEntry::EventType::Unknown);

    /**
     * @brief Verify the integrity of the audit log file.
     * @return True if the log appears unmodified since last verification/checksum.
     */
    bool verifyIntegrity() const;

    /**
     * @brief Sign the current audit log file with a digital signature (if enabled).
     * @return True if signing was successful.
     */
    bool signLog() const;

signals:
    /**
     * @brief Emitted when a new event is logged.
     * @param entry The new audit entry.
     */
    void eventLogged(const QuantilyxDoc::AuditEntry& entry);

    /**
     * @brief Emitted when the log file is rotated due to size limits.
     */
    void logRotated();

    /**
     * @brief Emitted when integrity check fails.
     * @param message Error message.
     */
    void integrityCheckFailed(const QString& message);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_AUDITTRAIL_H