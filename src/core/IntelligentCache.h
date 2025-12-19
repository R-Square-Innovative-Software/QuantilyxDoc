/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_INTELLIGENTCACHE_H
#define QUANTILYX_INTELLIGENTCACHE_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QMutex>
#include <QReadWriteLock>
#include <QDateTime>
#include <QVariant>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

/**
 * @brief A cache that uses predictive algorithms and access patterns to optimize storage.
 * 
 * Extends basic caching (like PageCache) by predicting which items are likely to be
 * accessed next and pre-loading them, while evicting less likely items based on
 * more sophisticated models than simple LRU.
 */
class IntelligentCache : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Information stored about each cached item.
     */
    struct CachedItem {
        QVariant data;              // The cached data
        qint64 sizeBytes;           // Size of the data
        QDateTime lastAccessTime;   // Last time accessed
        QDateTime creationTime;     // When it was cached
        int accessCount;            // How many times accessed
        qreal priority;             // Calculated priority for retention
        QString key;                // The cache key (stored here for potential analysis)
        QVariantMap metadata;       // Additional metadata about the item
    };

    /**
     * @brief Policy for evicting items when the cache is full.
     */
    enum class EvictionPolicy {
        LRU,            // Least Recently Used
        LFU,            // Least Frequently Used
        Priority,       // Based on calculated priority
        Predictive      // Based on access pattern prediction
    };

    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit IntelligentCache(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~IntelligentCache() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global IntelligentCache instance.
     */
    static IntelligentCache& instance();

    /**
     * @brief Store an item in the cache.
     * @param key Unique key for the item.
     * @param data The data to cache.
     * @param sizeHint Optional size hint in bytes.
     * @param metadata Optional metadata.
     * @return True if the item was successfully cached.
     */
    bool put(const QString& key, const QVariant& data, qint64 sizeHint = 0, const QVariantMap& metadata = QVariantMap());

    /**
     * @brief Retrieve an item from the cache.
     * @param key The key of the item.
     * @return The cached data, or an invalid QVariant if not found.
     */
    QVariant get(const QString& key);

    /**
     * @brief Check if an item exists in the cache.
     * @param key The key of the item.
     * @return True if the item exists.
     */
    bool contains(const QString& key) const;

    /**
     * @brief Remove an item from the cache.
     * @param key The key of the item to remove.
     * @return True if the item was removed.
     */
    bool remove(const QString& key);

    /**
     * @brief Clear the entire cache.
     */
    void clear();

    /**
     * @brief Get the maximum size of the cache in bytes.
     * @return Maximum size.
     */
    qint64 maxSizeBytes() const;

    /**
     * @brief Set the maximum size of the cache in bytes.
     * @param size Maximum size in bytes.
     */
    void setMaxSizeBytes(qint64 size);

    /**
     * @brief Get the current size of the cache in bytes.
     * @return Current size.
     */
    qint64 currentSizeBytes() const;

    /**
     * @brief Get the number of items currently in the cache.
     * @return Item count.
     */
    int itemCount() const;

    /**
     * @brief Get the current eviction policy.
     * @return Eviction policy.
     */
    EvictionPolicy evictionPolicy() const;

    /**
     * @brief Set the eviction policy.
     * @param policy New eviction policy.
     */
    void setEvictionPolicy(EvictionPolicy policy);

    /**
     * @brief Get a snapshot of all cached items (for debugging/statistics).
     * @return List of CachedItem structs.
     */
    QList<CachedItem> items() const;

    /**
     * @brief Hint to the cache that a specific key is likely to be accessed soon.
     * This can trigger pre-loading or adjust internal prediction models.
     * @param key The key expected to be accessed.
     */
    void hintAccess(const QString& key);

    /**
     * @brief Get cache statistics.
     * @return Map containing stats like hit rate, miss rate, size, count.
     */
    QVariantMap statistics() const;

    /**
     * @brief Calculate the memory footprint of a specific item.
     * @param item The item to calculate size for.
     * @return Estimated size in bytes.
     */
    static qint64 calculateItemSizeBytes(const CachedItem& item);

signals:
    /**
     * @brief Emitted when an item is added to the cache.
     * @param key The key of the item.
     * @param size Size of the item in bytes.
     */
    void itemAdded(const QString& key, qint64 size);

    /**
     * @brief Emitted when an item is removed from the cache (evicted or deleted).
     * @param key The key of the item.
     * @param size Size of the item in bytes.
     */
    void itemRemoved(const QString& key, qint64 size);

    /**
     * @brief Emitted when the cache size changes significantly.
     * @param currentSize Current cache size in bytes.
     * @param itemCount Number of items in cache.
     */
    void cacheSizeChanged(qint64 currentSize, int itemCount);

    /**
     * @brief Emitted when statistics are updated.
     */
    void statisticsChanged();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_INTELLIGENTCACHE_H