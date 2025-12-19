/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "RecentFiles.h"
#include "Logger.h"
#include <QSettings>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <algorithm> // For std::stable_sort

namespace QuantilyxDoc {

class RecentFiles::Private {
public:
    Private()
        : maxCount(10)
        , storagePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/recent_files.ini") {}

    QMutex mutex; // Protect access to recentFiles list
    QList<RecentFileInfo> recentFiles;
    int maxCount;
    QString storagePath;

    // Helper to find index of file in list
    int findFileIndex(const QString& path) const {
        for (int i = 0; i < recentFiles.size(); ++i) {
            if (QFileInfo(recentFiles[i].filePath).canonicalFilePath() ==
                QFileInfo(path).canonicalFilePath()) {
                return i;
            }
        }
        return -1;
    }

    // Helper to sort list by last access time (descending)
    void sortByLastAccess() {
        std::stable_sort(recentFiles.begin(), recentFiles.end(),
                         [](const RecentFileInfo& a, const RecentFileInfo& b) {
                             return a.lastAccessTime > b.lastAccessTime;
                         });
    }

    // Helper to prune list to maxCount
    void pruneToListSize() {
        while (recentFiles.size() > maxCount) {
            recentFiles.removeLast(); // Last item is oldest due to sorting
        }
    }
};

// Static instance pointer
RecentFiles* RecentFiles::s_instance = nullptr;

RecentFiles& RecentFiles::instance()
{
    if (!s_instance) {
        s_instance = new RecentFiles();
    }
    return *s_instance;
}

RecentFiles::RecentFiles(QObject* parent)
    : QObject(parent)
    , d(new Private())
{
    // Ensure the directory for the storage file exists
    QDir dir(QFileInfo(d->storagePath).absolutePath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

RecentFiles::~RecentFiles()
{
    save(); // Save on destruction
}

void RecentFiles::addFile(const QString& filePath, const QVariantMap& metadata)
{
    if (filePath.isEmpty()) return;

    QMutexLocker locker(&d->mutex);

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
         LOG_WARN("Cannot add non-existent file to recent files: " << filePath);
         return; // Do not add non-existent files
    }

    int index = d->findFileIndex(filePath);
    QDateTime now = QDateTime::currentDateTime();

    if (index >= 0) {
        // File already exists, move it to the front and update metadata
        RecentFileInfo& info = d->recentFiles[index];
        d->recentFiles.move(index, 0); // Move to beginning
        info.lastAccessTime = now;
        info.fileSize = fileInfo.size();
        info.displayName = fileInfo.fileName(); // Update display name in case file was moved
        info.accessCount++; // Increment access count
        // Update other metadata from input map if present
        if (metadata.contains("DocumentType")) info.documentType = metadata["DocumentType"].toString();
        if (metadata.contains("Title")) info.lastKnownTitle = metadata["Title"].toString();
    } else {
        // New file, add it to the front
        RecentFileInfo info;
        info.filePath = fileInfo.canonicalFilePath(); // Store canonical path
        info.lastAccessTime = now;
        info.fileSize = fileInfo.size();
        info.displayName = fileInfo.fileName();
        info.accessCount = 1;
        // Populate from metadata map
        if (metadata.contains("DocumentType")) info.documentType = metadata["DocumentType"].toString();
        if (metadata.contains("Title")) info.lastKnownTitle = metadata["Title"].toString();
        d->recentFiles.prepend(info);
    }

    // Prune list to max size
    d->pruneToListSize();

    emit recentFilesChanged();
    emit fileAdded(d->recentFiles.first());
}

void RecentFiles::removeFile(const QString& filePath)
{
    if (filePath.isEmpty()) return;

    QMutexLocker locker(&d->mutex);
    int index = d->findFileIndex(filePath);
    if (index >= 0) {
        RecentFileInfo removedInfo = d->recentFiles.takeAt(index);
        emit recentFilesChanged();
        emit fileRemoved(removedInfo.filePath);
    }
}

void RecentFiles::clear()
{
    QMutexLocker locker(&d->mutex);
    d->recentFiles.clear();
    emit recentFilesChanged();
    emit cleared();
}

QStringList RecentFiles::filePaths() const
{
    QMutexLocker locker(&d->mutex);
    QStringList paths;
    paths.reserve(d->recentFiles.size());
    for (const auto& info : d->recentFiles) {
        paths.append(info.filePath);
    }
    return paths;
}

QList<RecentFileInfo> RecentFiles::fileInfos() const
{
    QMutexLocker locker(&d->mutex);
    // Return a copy, sorted by access time (most recent first)
    QList<RecentFileInfo> sortedList = d->recentFiles;
    std::stable_sort(sortedList.begin(), sortedList.end(),
                     [](const RecentFileInfo& a, const RecentFileInfo& b) {
                         return a.lastAccessTime > b.lastAccessTime;
                     });
    return sortedList;
}

int RecentFiles::maxRecentFiles() const
{
    QMutexLocker locker(&d->mutex);
    return d->maxCount;
}

void RecentFiles::setMaxRecentFiles(int count)
{
    if (count < 0) return; // Invalid count

    QMutexLocker locker(&d->mutex);
    if (d->maxCount != count) {
        d->maxCount = count;
        d->pruneToListSize(); // Adjust list size if necessary
        emit recentFilesChanged();
    }
}

bool RecentFiles::containsFile(const QString& filePath) const
{
    QMutexLocker locker(&d->mutex);
    return d->findFileIndex(filePath) >= 0;
}

RecentFileInfo RecentFiles::fileInfo(const QString& filePath) const
{
    QMutexLocker locker(&d->mutex);
    int index = d->findFileIndex(filePath);
    if (index >= 0) {
        return d->recentFiles[index];
    }
    return RecentFileInfo(); // Return invalid/default info
}

void RecentFiles::load()
{
    QSettings settings(d->storagePath, QSettings::IniFormat);
    QMutexLocker locker(&d->mutex);

    d->recentFiles.clear();
    int size = settings.beginReadArray("RecentFiles");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        RecentFileInfo info;
        info.filePath = settings.value("FilePath").toString();
        info.lastAccessTime = settings.value("LastAccessTime").toDateTime();
        info.fileSize = settings.value("FileSize").toLongLong();
        info.displayName = settings.value("DisplayName").toString();
        info.documentType = settings.value("DocumentType").toString();
        info.lastKnownTitle = settings.value("LastKnownTitle").toString();
        info.accessCount = settings.value("AccessCount", 1).toInt();

        // Only add if the file actually exists
        if (QFileInfo(info.filePath).exists()) {
            d->recentFiles.append(info);
        } else {
            LOG_DEBUG("Skipping non-existent file from recent files list: " << info.filePath);
        }
    }
    settings.endArray();

    // Sort after loading to ensure order consistency
    d->sortByLastAccess();
    // Prune in case the stored count exceeded the current max
    d->pruneToListSize();

    emit recentFilesChanged();
}

void RecentFiles::save()
{
    QSettings settings(d->storagePath, QSettings::IniFormat);
    QMutexLocker locker(&d->mutex);

    settings.beginWriteArray("RecentFiles");
    for (int i = 0; i < d->recentFiles.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("FilePath", d->recentFiles[i].filePath);
        settings.setValue("LastAccessTime", d->recentFiles[i].lastAccessTime);
        settings.setValue("FileSize", d->recentFiles[i].fileSize);
        settings.setValue("DisplayName", d->recentFiles[i].displayName);
        settings.setValue("DocumentType", d->recentFiles[i].documentType);
        settings.setValue("LastKnownTitle", d->recentFiles[i].lastKnownTitle);
        settings.setValue("AccessCount", d->recentFiles[i].accessCount);
    }
    settings.endArray();
    settings.sync(); // Force write to disk
}

void RecentFiles::validate()
{
    QMutexLocker locker(&d->mutex);
    bool changed = false;
    auto it = d->recentFiles.begin();
    while (it != d->recentFiles.end()) {
        if (!QFileInfo(it->filePath).exists()) {
            LOG_DEBUG("Removing non-existent file from recent files: " << it->filePath);
            it = d->recentFiles.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    if (changed) {
        emit recentFilesChanged();
    }
}

QString RecentFiles::storagePath() const
{
    QMutexLocker locker(&d->mutex);
    return d->storagePath;
}

void RecentFiles::setStoragePath(const QString& path)
{
    QMutexLocker locker(&d->mutex);
    if (d->storagePath != path) {
        d->storagePath = path;
        // Ensure directory exists for new path
        QDir dir(QFileInfo(path).absolutePath());
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        LOG_INFO("Recent files storage path changed to: " << path);
    }
}

} // namespace QuantilyxDoc