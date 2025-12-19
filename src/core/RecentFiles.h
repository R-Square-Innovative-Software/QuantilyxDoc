/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_RECENTFILES_H
#define QUANTILYX_RECENTFILES_H

#include <QObject>
#include <QStringList>
#include <QHash>
#include <QDateTime>
#include <QMetaType>
#include <memory>

namespace QuantilyxDoc {

/**
 * @brief Stores information about a recent file.
 */
struct RecentFileInfo {
    QString filePath;           // Absolute path to the file
    QDateTime lastAccessTime;   // Time the file was last opened/accessed
    qint64 fileSize;            // Size of the file at last access
    QString displayName;        // Display name (filename or custom title)
    QString documentType;       // Detected document type (e.g., "PDF", "EPUB")
    QString lastKnownTitle;     // Title from document metadata (if available)
    int accessCount;            // How many times the file was accessed

    // Needed for QMetaType registration
    RecentFileInfo() : fileSize(0), accessCount(0) {}
    RecentFileInfo(const QString& path)
        : filePath(path), fileSize(0), accessCount(1) {}
};

Q_DECLARE_METATYPE(QuantilyxDoc::RecentFileInfo)

/**
 * @brief Manages the list of recently opened files.
 *
 * Maintains a list of recently accessed documents, stores metadata,
 * and provides signals for UI updates.
 */
class RecentFiles : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit RecentFiles(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~RecentFiles() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global RecentFiles instance.
     */
    static RecentFiles& instance();

    /**
     * @brief Add a file to the recent files list.
     * @param filePath Path to the file.
     * @param metadata Optional metadata to store with the file.
     */
    void addFile(const QString& filePath, const QVariantMap& metadata = QVariantMap());

    /**
     * @brief Remove a file from the recent files list.
     * @param filePath Path to the file to remove.
     */
    void removeFile(const QString& filePath);

    /**
     * @brief Clear the entire recent files list.
     */
    void clear();

    /**
     * @brief Get the list of recent file paths.
     * @return List of file paths, ordered from most recent to oldest.
     */
    QStringList filePaths() const;

    /**
     * @brief Get detailed information for all recent files.
     * @return List of RecentFileInfo structures.
     */
    QList<RecentFileInfo> fileInfos() const;

    /**
     * @brief Get the maximum number of recent files to store.
     * @return Maximum count.
     */
    int maxRecentFiles() const;

    /**
     * @brief Set the maximum number of recent files to store.
     * @param count New maximum count.
     */
    void setMaxRecentFiles(int count);

    /**
     * @brief Check if a file is in the recent files list.
     * @param filePath Path to the file.
     * @return True if the file is in the list.
     */
    bool containsFile(const QString& filePath) const;

    /**
     * @brief Get information for a specific file.
     * @param filePath Path to the file.
     * @return RecentFileInfo structure, or invalid one if not found.
     */
    RecentFileInfo fileInfo(const QString& filePath) const;

    /**
     * @brief Load recent files list from persistent storage.
     */
    void load();

    /**
     * @brief Save recent files list to persistent storage.
     */
    void save();

    /**
     * @brief Validate the recent files list by checking if files still exist.
     * Removes entries for files that no longer exist.
     */
    void validate();

    /**
     * @brief Get the path to the recent files storage file (e.g., INI file).
     * @return Storage file path.
     */
    QString storagePath() const;

    /**
     * @brief Set the path to the recent files storage file.
     * @param path Storage file path.
     */
    void setStoragePath(const QString& path);

signals:
    /**
     * @brief Emitted when the recent files list changes.
     */
    void recentFilesChanged();

    /**
     * @brief Emitted when a file is added to the list.
     * @param fileInfo Information about the added file.
     */
    void fileAdded(const QuantilyxDoc::RecentFileInfo& fileInfo);

    /**
     * @brief Emitted when a file is removed from the list.
     * @param filePath Path of the removed file.
     */
    void fileRemoved(const QString& filePath);

    /**
     * @brief Emitted when the list is cleared.
     */
    void cleared();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_RECENTFILES_H