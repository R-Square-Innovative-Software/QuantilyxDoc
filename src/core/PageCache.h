/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_PAGECACHE_H
#define QUANTILYX_PAGECACHE_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QImage>
#include <QSize>
#include <memory>

namespace QuantilyxDoc {

class Page;

/**
 * @brief Manages cached page renderings for performance.
 *
 * Implements an LRU (Least Recently Used) cache to store pre-rendered
 * page images at various resolutions and zoom levels.
 */
class PageCache : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Unique identifier for a cached page image.
     * Combines document ID, page index, zoom level, and rotation.
     */
    struct CacheKey {
        quintptr documentId;
        int pageIndex;
        qreal zoomLevel;
        int rotation; // 0, 90, 180, 270
        QSize targetSize; // Size in pixels requested

        // Required for use as a hash key
        bool operator==(const CacheKey& other) const {
            return documentId == other.documentId &&
                   pageIndex == other.pageIndex &&
                   qFuzzyCompare(zoomLevel, other.zoomLevel) &&
                   rotation == other.rotation &&
                   targetSize == other.targetSize;
        }
    };

    /**
     * @brief Hash function for CacheKey.
     */
    struct CacheKeyHash {
        std::size_t operator()(const CacheKey& k) const {
            // Combine hashes of all key components
            std::size_t h1 = std::hash<quintptr>{}(k.documentId);
            std::size_t h2 = std::hash<int>{}(k.pageIndex);
            std::size_t h3 = std::hash<double>{}(static_cast<double>(k.zoomLevel));
            std::size_t h4 = std::hash<int>{}(k.rotation);
            std::size_t h5 = std::hash<size_t>{}(qHash(k.targetSize));
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
        }
    };

    /**
     * @brief Information stored about each cached item.
     */
    struct CachedItem {
        QImage image;
        qint64 timestamp; // For LRU eviction
        int accessCount; // For LRU or other policies
    };

    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit PageCache(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~PageCache() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global PageCache instance.
     */
    static PageCache& instance();

    /**
     * @brief Retrieve a cached page image.
     * @param key The cache key identifying the page and its rendering parameters.
     * @return The cached image, or a null QImage if not found.
     */
    QImage get(const CacheKey& key);

    /**
     * @brief Store a page image in the cache.
     * @param key The cache key identifying the page and its rendering parameters.
     * @param image The rendered image to store.
     */
    void put(const CacheKey& key, const QImage& image);

    /**
     * @brief Check if a specific page image is cached.
     * @param key The cache key to check.
     * @return True if the image exists in the cache.
     */
    bool contains(const Cache-Key& key) const;

    /**
     * @brief Clear all cached images for a specific document.
     * @param documentId The ID of the document to clear.
     */
    void clearForDocument(quintptr documentId);

    /**
     * @brief Clear the entire cache.
     */
    void clear();

    /**
     * @brief Get the maximum cache size in bytes.
     * @return Maximum size in bytes.
     */
    qint64 maxSizeBytes() const;

    /**
     * @brief Set the maximum cache size in bytes.
     * @param size New maximum size in bytes.
     */
    void setMaxSizeBytes(qint64 size);

    /**
     * @brief Get the current cache size in bytes.
     * @return Current size in bytes.
     */
    qint64 currentSizeBytes() const;

    /**
     * @brief Get the number of items currently cached.
     * @return Number of cached items.
     */
    int itemCount() const;

    /**
     * @brief Evict least recently used items if the cache exceeds max size.
     */
    void evictIfNecessary();

    /**
     * @brief Calculate memory usage of a single image.
     * @param image The image to calculate size for.
     * @return Size in bytes.
     */
    static qint64 calculateImageSizeBytes(const QImage& image);

signals:
    /**
     * @brief Emitted when cache statistics change.
     * @param currentSize Current cache size in bytes.
     * @param itemCount Number of items in cache.
     */
    void statisticsChanged(qint64 currentSize, int itemCount);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

// QHash specialization for CacheKey
QT_BEGIN_NAMESPACE
uint qHash(const QuantilyxDoc::PageCache::CacheKey& key, uint seed = 0);
QT_END_NAMESPACE


#endif // QUANTILYX_PAGECACHE_H