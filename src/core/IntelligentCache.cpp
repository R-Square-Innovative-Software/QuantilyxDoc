/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "IntelligentCache.h"
#include "Logger.h"
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QVariant>
#include <QImage> // For size calculation example
#include <QDebug>
#include <algorithm> // For std::sort, std::find_if

namespace QuantilyxDoc {

class IntelligentCache::Private {
public:
    Private(IntelligentCache* q_ptr)
        : q(q_ptr), maxSizeBytes(50 * 1024 * 1024), currentSizeBytes(0), // 50 MB default
          evictionPolicy(EvictionPolicy::LRU) {} // Default policy

    IntelligentCache* q;
    mutable QReadWriteLock dataLock; // Use R/W lock for better concurrent access
    QHash<QString, CachedItem> cacheData;
    qint64 maxSizeBytes;
    qint64 currentSizeBytes;
    EvictionPolicy evictionPolicy;
    // For predictive models, we might need access logs, graph structures, etc.
    // For this stub, we'll implement LRU and a simple priority bump on access.
    // A real predictive model would be much more complex.

    // Helper to evict items based on current policy
    void evictIfNeeded() {
        // This should be called after acquiring a Write lock on dataLock
        while (currentSizeBytes > maxSizeBytes && !cacheData.isEmpty()) {
            QString keyToEvict;
            switch (evictionPolicy) {
                case EvictionPolicy::LRU: {
                    // Find oldest accessed item
                    QDateTime oldestTime = QDateTime::currentDateTime();
                    for (auto it = cacheData.constBegin(); it != cacheData.constEnd(); ++it) {
                        if (it.value().lastAccessTime < oldestTime) {
                            oldestTime = it.value().lastAccessTime;
                            keyToEvict = it.key();
                        }
                    }
                    break;
                }
                case EvictionPolicy::LFU: {
                    // Find least frequently accessed item
                    int minCount = INT_MAX;
                    for (auto it = cacheData.constBegin(); it != cacheData.constEnd(); ++it) {
                        if (it.value().accessCount < minCount) {
                            minCount = it.value().accessCount;
                            keyToEvict = it.key();
                        }
                    }
                    break;
                }
                case EvictionPolicy::Priority: {
                    // Find item with lowest priority
                    qreal minPriority = std::numeric_limits<qreal>::max();
                    for (auto it = cacheData.constBegin(); it != cacheData.constEnd(); ++it) {
                        if (it.value().priority < minPriority) {
                            minPriority = it.value().priority;
                            keyToEvict = it.key();
                        }
                    }
                    break;
                }
                case EvictionPolicy::Predictive: {
                    // A stub for a predictive model.
                    // In reality, this would use access history, graph traversal, ML models, etc.
                    // to predict which item is least likely to be needed.
                    // For now, fall back to LRU.
                    QDateTime oldestTime = QDateTime::currentDateTime();
                    for (auto it = cacheData.constBegin(); it != cacheData.constEnd(); ++it) {
                        if (it.value().lastAccessTime < oldestTime) {
                            oldestTime = it.value().lastAccessTime;
                            keyToEvict = it.key();
                        }
                    }
                    break;
                }
            }

            if (!keyToEvict.isEmpty()) {
                CachedItem item = cacheData.take(keyToEvict);
                currentSizeBytes -= item.sizeBytes;
                LOG_DEBUG("Evicted item from cache: " << keyToEvict << ", Size: " << item.sizeBytes);
                emit q->itemRemoved(keyToEvict, item.sizeBytes);
            } else {
                // Should not happen if cacheData is not empty
                LOG_WARN("Cache size exceeded limit but no item found for eviction!");
                break; // Avoid infinite loop
            }
        }
        emit q->cacheSizeChanged(currentSizeBytes, cacheData.size());
    }

    // Helper to calculate item size (stub implementation)
    static qint64 calculateItemSizeBytes(const CachedItem& item) {
        qint64 size = 0;
        if (item.data.canConvert<QString>()) {
            size += item.data.toString().size() * sizeof(QChar);
        } else if (item.data.canConvert<QImage>()) {
            QImage img = item.data.value<QImage>();
            size += img.sizeInBytes();
        } else if (item.data.canConvert<QByteArray>()) {
            size += item.data.toByteArray().size();
        }
        // Add sizes for metadata map, key string, etc.
        size += item.key.size() * sizeof(QChar);
        for (auto it = item.metadata.constBegin(); it != item.metadata.constEnd(); ++it) {
             size += it.key().size() * sizeof(QChar) + it.value().sizeInBytes(); // Approximation
        }
        return size;
    }
};

// Static instance pointer
IntelligentCache* IntelligentCache::s_instance = nullptr;

IntelligentCache& IntelligentCache::instance()
{
    if (!s_instance) {
        s_instance = new IntelligentCache();
    }
    return *s_instance;
}

IntelligentCache::IntelligentCache(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    // Initialization happens in Private constructor
}

IntelligentCache::~IntelligentCache()
{
    // d->cacheData will be cleared automatically
}

bool IntelligentCache::put(const QString& key, const QVariant& data, qint64 sizeHint, const QVariantMap& metadata)
{
    QWriteLocker locker(&d->dataLock);

    qint64 itemSize = sizeHint > 0 ? sizeHint : Private::calculateItemSizeBytes({data, 0, QDateTime(), QDateTime(), 0, 0.0, key, metadata});

    // Check if replacing an existing item
    auto existingIt = d->cacheData.find(key);
    if (existingIt != d->cacheData.end()) {
        d->currentSizeBytes -= existingIt->sizeBytes;
        d->cacheData.erase(existingIt);
        LOG_DEBUG("Replacing existing item in cache: " << key);
    }

    CachedItem item;
    item.data = data;
    item.sizeBytes = itemSize;
    item.lastAccessTime = QDateTime::currentDateTime();
    item.creationTime = item.lastAccessTime; // New item
    item.accessCount = 1;
    item.priority = 1.0; // Default priority
    item.key = key;
    item.metadata = metadata;

    d->cacheData.insert(key, item);
    d->currentSizeBytes += itemSize;

    // Check if we need to evict
    d->evictIfNeeded();

    emit itemAdded(key, itemSize);
    return true;
}

QVariant IntelligentCache::get(const QString& key)
{
    QWriteLocker locker(&d->dataLock); // Write lock because we modify access time/count

    auto it = d->cacheData.find(key);
    if (it != d->cacheData.end()) {
        // Update access stats
        it->lastAccessTime = QDateTime::currentDateTime();
        it->accessCount++;
        // Optionally bump priority based on access or prediction model
        it->priority += 0.1; // Simple bump

        emit q->statisticsChanged(); // Access stats changed
        return it->data;
    }
    return QVariant(); // Not found
}

bool IntelligentCache::contains(const QString& key) const
{
    QReadLocker locker(&d->dataLock);
    return d->cacheData.contains(key);
}

bool IntelligentCache::remove(const QString& key)
{
    QWriteLocker locker(&d->dataLock);
    auto it = d->cacheData.find(key);
    if (it != d->cacheData.end()) {
        qint64 size = it->sizeBytes;
        d->currentSizeBytes -= size;
        d->cacheData.erase(it);
        emit itemRemoved(key, size);
        emit cacheSizeChanged(d->currentSizeBytes, d->cacheData.size());
        return true;
    }
    return false;
}

void IntelligentCache::clear()
{
    QWriteLocker locker(&d->dataLock);
    qint64 oldSize = d->currentSizeBytes;
    int oldCount = d->cacheData.size();
    d->cacheData.clear();
    d->currentSizeBytes = 0;
    LOG_DEBUG("Cleared entire cache. Removed " << oldCount << " items, freed " << oldSize << " bytes.");
    emit cacheSizeChanged(0, 0);
}

qint64 IntelligentCache::maxSizeBytes() const
{
    QReadLocker locker(&d->dataLock);
    return d->maxSizeBytes;
}

void IntelligentCache::setMaxSizeBytes(qint64 size)
{
    if (size <= 0) return;
    QWriteLocker locker(&d->dataLock);
    if (d->maxSizeBytes != size) {
        d->maxSizeBytes = size;
        LOG_INFO("Cache max size changed to " << size << " bytes. Triggering eviction if necessary.");
        d->evictIfNeeded(); // Enforce new limit
    }
}

qint64 IntelligentCache::currentSizeBytes() const
{
    QReadLocker locker(&d->dataLock);
    return d->currentSizeBytes;
}

int IntelligentCache::itemCount() const
{
    QReadLocker locker(&d->dataLock);
    return d->cacheData.size();
}

IntelligentCache::EvictionPolicy IntelligentCache::evictionPolicy() const
{
    QReadLocker locker(&d->dataLock);
    return d->evictionPolicy;
}

void IntelligentCache::setEvictionPolicy(EvictionPolicy policy)
{
    QWriteLocker locker(&d->dataLock);
    if (d->evictionPolicy != policy) {
        d->evictionPolicy = policy;
        LOG_INFO("Cache eviction policy changed to " << static_cast<int>(policy));
        // Consider re-sorting/triggering eviction based on new policy immediately?
        // This could be expensive. Maybe just apply the new policy on next put/evict.
    }
}

QList<IntelligentCache::CachedItem> IntelligentCache::items() const
{
    QReadLocker locker(&d->dataLock);
    QList<CachedItem> list;
    list.reserve(d->cacheData.size());
    for (const auto& item : d->cacheData) {
        list.append(item);
    }
    return list;
}

void IntelligentCache::hintAccess(const QString& key)
{
    QWriteLocker locker(&d->dataLock);
    auto it = d->cacheData.find(key);
    if (it != d->cacheData.end()) {
        // Bump priority or move to front based on policy
        it->priority += 0.5; // Significant bump for a hint
        it->lastAccessTime = QDateTime::currentDateTime(); // Update time
        LOG_DEBUG("Hinted access for item: " << key);
    } else {
        // Item not in cache, could trigger a pre-load based on prediction
        LOG_DEBUG("Hinted access for non-existent item: " << key << ". Could trigger pre-load.");
    }
}

QVariantMap IntelligentCache::statistics() const
{
    QReadLocker locker(&d->dataLock);
    QVariantMap stats;
    stats["maxSizeBytes"] = d->maxSizeBytes;
    stats["currentSizeBytes"] = d->currentSizeBytes;
    stats["itemCount"] = d->cacheData.size();
    stats["evictionPolicy"] = static_cast<int>(d->evictionPolicy);
    // Could add hit/miss rates if tracked separately
    return stats;
}

qint64 IntelligentCache::calculateItemSizeBytes(const CachedItem& item)
{
    return Private::calculateItemSizeBytes(item);
}

} // namespace QuantilyxDoc