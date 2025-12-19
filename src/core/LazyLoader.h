/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_LAZYLOADER_H
#define QUANTILYX_LAZYLOADER_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QRunnable>
#include <QThreadPool>
#include <functional>
#include <memory>

namespace QuantilyxDoc {

class Document;
class Page;

/**
 * @brief Manages lazy loading of document resources (e.g., pages, images, fonts).
 * 
 * Loads resources only when they are first accessed or are likely to be accessed soon,
 * using background threads to avoid blocking the UI. Integrates with caches like
 * IntelligentCache for optimal performance.
 */
class LazyLoader : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Type of resource to be loaded lazily.
     */
    enum class ResourceType {
        PageContent,
        PageThumbnail,
        EmbeddedImage,
        Font,
        Annotation,
        FormField
    };

    /**
     * @brief A request for a resource to be loaded.
     */
    struct LoadRequest {
        QString key;                    // Unique identifier for the resource
        ResourceType type;              // Type of resource
        QVariantMap parameters;         // Additional parameters (e.g., page index, dimensions)
        std::function<void(QVariant)> onSuccess; // Callback on successful load
        std::function<void(QString)> onError;   // Callback on error
        qint64 priority;                // Priority of the request (higher = more urgent)
        QDateTime requestTime;          // When the request was made

        LoadRequest() : priority(0) {}
    };

    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit LazyLoader(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~LazyLoader() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global LazyLoader instance.
     */
    static LazyLoader& instance();

    /**
     * @brief Queue a request to load a resource lazily.
     * @param request The load request.
     */
    void queueRequest(const LoadRequest& request);

    /**
     * @brief Queue a request using convenience parameters.
     * @param key Unique identifier.
     * @param type Type of resource.
     * @param params Parameters for loading.
     * @param onSuccess Callback for success.
     * @param onError Callback for error.
     * @param priority Priority of the request.
     */
    void queueRequest(const QString& key, ResourceType type, const QVariantMap& params,
                      std::function<void(QVariant)> onSuccess,
                      std::function<void(QString)> onError,
                      qint64 priority = 0);

    /**
     * @brief Cancel a pending load request by its key.
     * @param key The key of the request to cancel.
     * @return True if the request was found and canceled.
     */
    bool cancelRequest(const QString& key);

    /**
     * @brief Cancel all pending requests.
     */
    void cancelAllRequests();

    /**
     * @brief Get the number of requests currently queued.
     * @return Queued request count.
     */
    int queuedRequestCount() const;

    /**
     * @brief Get the number of requests currently being processed.
     * @return Active request count.
     */
    int activeRequestCount() const;

    /**
     * @brief Set the maximum number of concurrent loading threads.
     * @param count Maximum thread count.
     */
    void setMaxConcurrentLoads(int count);

    /**
     * @brief Get the maximum number of concurrent loading threads.
     * @return Maximum thread count.
     */
    int maxConcurrentLoads() const;

    /**
     * @brief Preload resources based on a prediction (e.g., next few pages).
     * @param resourceKeys List of keys to preload.
     * @param priority Priority for preloading tasks.
     */
    void preload(const QStringList& resourceKeys, qint64 priority = 1);

    /**
     * @brief Hint that a specific resource will be needed soon.
     * This can bump its priority in the queue.
     * @param key The key of the resource.
     */
    void hintResourceNeeded(const QString& key);

    /**
     * @brief Clear the loader's internal state (requests, predictions).
     */
    void clear();

    /**
     * @brief Get statistics about loading performance.
     * @return Map containing stats like average load time, success rate, queue depth.
     */
    QVariantMap statistics() const;

signals:
    /**
     * @brief Emitted when a resource is successfully loaded.
     * @param key The key of the loaded resource.
     * @param data The loaded data.
     */
    void resourceLoaded(const QString& key, const QVariant& data);

    /**
     * @brief Emitted when a resource fails to load.
     * @param key The key of the failed resource.
     * @param error Error message.
     */
    void resourceLoadFailed(const QString& key, const QString& error);

    /**
     * @brief Emitted when the queue status (counts) changes.
     * @param queuedCount Number of queued requests.
     * @param activeCount Number of active requests.
     */
    void queueStatusChanged(int queuedCount, int activeCount);

private slots:
    void processNextRequest(); // Internal slot to handle the queue

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_LAZYLOADER_H