/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_BACKUPMANAGER_H
#define QUANTILYX_BACKUPMANAGER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QDateTime>
#include <QDir>
#include <memory>

namespace QuantilyxDoc {

class Document;

struct BackupInfo {
    QString filePath;       // Path to the backup file
    QDateTime timestamp;    // When the backup was created
    qint64 originalSize;    // Size of the original document at backup time
    QString documentTitle;  // Title of the document for reference
};

/**
 * @brief Manages automatic saving and backup of documents.
 *
 * Handles periodic auto-saves and maintains backup copies in a designated
 * directory. Can restore documents from backups if the original is lost/corrupted.
 */
class BackupManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit BackupManager(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~BackupManager() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global BackupManager instance.
     */
    static BackupManager& instance();

    /**
     * @brief Start monitoring a document for auto-save.
     * @param doc The document to monitor.
     */
    void watchDocument(Document* doc);

    /**
     * @brief Stop monitoring a document.
     * @param doc The document to stop monitoring.
     */
    void unwatchDocument(Document* doc);

    /**
     * @brief Perform an immediate auto-save for a specific document.
     * @param doc The document to save.
     * @return True if save was successful.
     */
    bool saveNow(Document* doc);

    /**
     * @brief Get the backup directory path.
     * @return Path to the backup directory.
     */
    QDir backupDirectory() const;

    /**
     * @brief Set the backup directory path.
     * @param dir Path to the backup directory.
     */
    void setBackupDirectory(const QDir& dir);

    /**
     * @brief Get the auto-save interval in seconds.
     * @return Interval in seconds.
     */
    int autoSaveIntervalSeconds() const;

    /**
     * @brief Set the auto-save interval in seconds.
     * @param seconds New interval.
     */
    void setAutoSaveIntervalSeconds(int seconds);

    /**
     * @brief Get the maximum number of backups to keep per document.
     * @return Maximum backup count.
     */
    int maxBackupsPerDocument() const;

    /**
     * @brief Set the maximum number of backups to keep per document.
     * @param count New maximum count.
     */
    void setMaxBackupsPerDocument(int count);

    /**
     * @brief Check if backup is enabled.
     * @return True if backup is active.
     */
    bool isEnabled() const;

    /**
     * @brief Enable or disable backup functionality.
     * @param enabled Whether to enable backup.
     */
    void setEnabled(bool enabled);

    /**
     * @brief Get list of backups for a specific document.
     * @param doc The document.
     * @return List of BackupInfo structures.
     */
    QList<BackupInfo> getBackupsForDocument(Document* doc) const;

    /**
     * @brief Restore a document from a specific backup file.
     * @param backupFilePath Path to the backup file.
     * @param targetDocumentPath Path where the restored document should be saved.
     * @return True if restoration was successful.
     */
    bool restoreFromBackup(const QString& backupFilePath, const QString& targetDocumentPath);

    /**
     * @brief Clean up old backups based on retention policy.
     */
    void cleanupOldBackups();

    /**
     * @brief Purge all backups for a specific document.
     * @param doc The document.
     */
    void purgeBackupsForDocument(Document* doc);

    /**
     * @brief Purge all backups for all documents.
     */
    void purgeAllBackups();

signals:
    /**
     * @brief Emitted when a backup is successfully created.
     * @param doc The document that was backed up.
     * @param backupPath Path to the created backup file.
     */
    void backupCreated(Document* doc, const QString& backupPath);

    /**
     * @brief Emitted when a backup fails.
     * @param doc The document for which backup failed.
     * @param error Error message.
     */
    void backupFailed(Document* doc, const QString& error);

    /**
     * @brief Emitted when a document is restored from backup.
     * @param originalPath Original document path.
     * @param backupPath Path to the backup used.
     */
    void documentRestored(const QString& originalPath, const QString& backupPath);

    /**
     * @brief Emitted when old backups are cleaned up.
     */
    void cleanupFinished();

private slots:
    void onAutoSaveTimer();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_BACKUPMANAGER_H