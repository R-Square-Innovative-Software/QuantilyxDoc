/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "PageCache.h"
#include "Page.h"
#include "Document.h"
#include <QMutex>
#include <QMutexLocker>
#include <QImage>
#include <QDebug>
#include <QFileInfo>
#include <algorithm> // For std::find_if

// Define the QHash specialization outside the namespace
uint qHash(const QuantilyxDoc::PageCache::CacheKey& key, uint seed)
{
    return QuantilyxDoc::PageCache::CacheKeyHash{}(key);
}

namespace QuantilyxDoc {

class PageCache::Private {
public:
    Private()
        : maxSizeBytes(50 * 1024 * 1024) // Default 50 MB
        , currentSizeBytes(0) {}

    mutable QMutex mutex; // Protect access to cache maps
    QHash<CacheKey, CachedItem, CacheKeyHash> cacheMap;
    QQueue<CacheKey> lruQueue; // Tracks access order
    qint64 maxSizeBytes;
    qint64 currentSizeBytes;

    // Helper to remove an item from the cache and update size
    void removeItem(const CacheKey& key) {
        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) {
            currentSizeBytes -= calculateImageSizeBytes(it->image);
            cacheMap.erase(it);
            
            // Remove from LRU queue
            lruQueue.removeAll(key);
        }
    }

    // Helper to calculate image size in bytes
    static qint64 calculateImageSizeBytes(const QImage& image) {
        if (image.isNull()) return 0;
        // Size = width * height * bytesPerPixel (format-dependent)
        return static_cast<qint64>(image.width()) * image.height() * (image.depth() / 8);
    }
};

// Static instance pointer
PageCache* PageCache::s_instance = nullptr;

PageCache& PageCache::instance()
{
    if (!s_instance) {
        s_instance = new PageCache();
    }
    return *s_instance;
}

PageCache::PageCache(QObject* parent)
    : QObject(parent)
    , d(new Private())
{
    // Ensure QCoreApplication exists before registering meta types
    if (qApp) {
        qRegisterMetaType<CacheKey>("QuantilyxDoc::PageCache::CacheKey");
    }
}

PageCache::~PageCache()
{
    clear(); // Ensure cache is cleared properly
}

QImage PageCache::get(const CacheKey& key)
{
    QMutexLocker locker(&d->mutex);
    auto it = d->cacheMap.find(key);
    if (it != d->cacheMap.end()) {
        // Update access count and timestamp for LRU
        it->accessCount++;
        it->timestamp = QDateTime::currentMSecsSinceEpoch();
        // Move key to end of LRU queue (most recently used)
        d->lruQueue.removeAll(key);
        d->lruQueue.enqueue(key);
        return it->image;
    }
    return QImage(); // Return null image if not found
}

void PageCache::put(const CacheKey& key, const QImage& image)
{
    QMutexLocker locker(&d->mutex);

    // Calculate size of new image
    qint64 imageSize = calculateImageSizeBytes(image);
    if (imageSize == 0) return; // Don't cache null images

    // Check if item already exists
    auto existingIt = d->cacheMap.find(key);
    if (existingIt != d->cacheMap.end()) {
        // Replace existing item and adjust size accordingly
        qint64 oldSize = calculateImageSizeBytes(existingIt->image);
        d->currentSizeBytes += (imageSize - oldSize);
        existingIt->image = image;
        existingIt->accessCount = 1;
        existingIt->timestamp = QDateTime::currentMSecsSinceEpoch();
        // Update LRU queue: remove old, add new
        d->lruQueue.removeAll(key);
        d->lruQueue.enqueue(key);
    } else {
        // Add new item
        CachedItem item;
        item.image = image;
        item.accessCount = 1;
        item.timestamp = QDateTime::currentMSecsSinceEpoch();
        d->cacheMap.insert(key, item);
        d->currentSizeBytes += imageSize;
        d->lruQueue.enqueue(key);
    }

    // Check if we exceed max size and evict if necessary
    evictIfNecessary();
    emit statisticsChanged(d->currentSizeBytes, d->cacheMap.size());
}

bool PageCache::contains(const CacheKey& key) const
{
    QMutexLocker locker(&d->mutex);
    return d->cacheMap.contains(key);
}

void PageCache::clearForDocument(quintptr documentId)
{
    QMutexLocker locker(&d->mutex);
    auto it = d->cacheMap.begin();
    while (it != d->cacheMap.end()) {
        if (it.key().documentId == documentId) {
            d->currentSizeBytes -= calculateImageSizeBytes(it.value().image);
            it = d->cacheMap.erase(it);
            // Also remove from LRU queue
            d->lruQueue.removeAll(it.key());
        } else {
            ++it;
        }
    }
    emit statisticsChanged(d->currentSizeBytes, d->cacheMap.size());
}

void PageCache::clear()
{
    QMutexLocker locker(&d->mutex);
    d->cacheMap.clear();
    d->lruQueue.clear();
    d->currentSizeBytes = 0;
    emit statisticsChanged(d->currentSizeBytes, 0);
}

qint64 PageCache::maxSizeBytes() const
{
    QMutexLocker locker(&d->mutex);
    return d->maxSizeBytes;
}

void PageCache::setMaxSizeBytes(qint64 size)
{
    QMutexLocker locker(&d->mutex);
    d->maxSizeBytes = size;
    evictIfNecessary(); // Enforce new limit immediately
}

qint64 PageCache::currentSizeBytes() const
{
    QMutexLocker locker(&d->mutex);
    return d->currentSizeBytes;
}

int PageCache::itemCount() const
{
    QMutexLocker locker(&d->mutex);
    return d->cacheMap.size();
}

void PageCache::evictIfNecessary()
{
    // Lock is assumed to be held by caller
    while (d->currentSizeBytes > d->maxSizeBytes && !d->lruQueue.isEmpty()) {
        CacheKey lruKey = d->lruQueue.dequeue();
        // The key might have been removed already by clearForDocument or put
        if (d->cacheMap.contains(lruKey)) {
            d->removeItem(lruKey);
        }
    }
}

qint64 PageCache::calculateImageSizeBytes(const QImage& image)
{
    return Private::calculateImageSizeBytes(image);
}

} // namespace QuantilyxDoc