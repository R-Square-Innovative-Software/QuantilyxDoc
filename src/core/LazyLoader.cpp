/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "LazyLoader.h"
#include "Logger.h"
#include "ThreadPool.h" // Use our custom ThreadPool
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <algorithm> // For std::sort, std::find, std::remove

namespace QuantilyxDoc {

class LazyLoader::Private {
public:
    Private(LazyLoader* q_ptr)
        : q(q_ptr), maxConcurrent(4), activeCount(0) {} // Default 4 threads

    LazyLoader* q;
    mutable QMutex mutex; // Protect access to queues and counts
    QQueue<LoadRequest> requestQueue;
    QSet<QString> activeRequests; // Keys of requests currently being processed
    int maxConcurrent;
    int activeCount;
    // Could add prediction structures here (e.g., access patterns, graphs)

    // Helper to find request index in queue by key
    int findRequestIndex(const QString& key) const {
        for (int i = 0; i < requestQueue.size(); ++i) {
            if (requestQueue[i].key == key) {
                return i;
            }
        }
        return -1;
    }

    // Helper to sort queue based on priority and request time (higher priority first, then FIFO for same priority)
    void sortQueue() {
        std::sort(requestQueue.begin(), requestQueue.end(), [](const LoadRequest& a, const LoadRequest& b) {
            if (a.priority != b.priority) {
                return a.priority > b.priority; // Higher priority first
            }
            return a.requestTime < b.requestTime; // FIFO for same priority
        });
    }
};

// Static instance pointer
LazyLoader* LazyLoader::s_instance = nullptr;

LazyLoader& LazyLoader::instance()
{
    if (!s_instance) {
        s_instance = new LazyLoader();
    }
    return *s_instance;
}

LazyLoader::LazyLoader(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    // Connect to ThreadPool signals to manage active count?
    // Or manage it internally as tasks start/finish.
    // For now, we'll manage activeCount internally in processNextRequest.
}

LazyLoader::~LazyLoader()
{
    cancelAllRequests(); // Attempt to cancel pending requests on destruction
    // Wait for active requests to finish? Debatable.
}

void LazyLoader::queueRequest(const LoadRequest& request)
{
    QMutexLocker locker(&d->mutex);

    // Check if already queued or active
    if (d->requestQueue.contains(request) || d->activeRequests.contains(request.key)) {
        LOG_DEBUG("Request already exists (queued or active): " << request.key);
        return; // Or maybe update the request? For now, ignore duplicates.
    }

    LoadRequest req = request;
    req.requestTime = QDateTime::currentDateTime();
    d->requestQueue.enqueue(req);
    d->sortQueue(); // Maintain priority order

    LOG_DEBUG("Queued lazy load request: " << request.key << " (Priority: " << request.priority << ")");

    // Potentially trigger processing if below concurrency limit
    // A better approach might be a QTimer or idle event to process the queue smoothly.
    // For now, we'll rely on processNextRequest being called externally or via a timer.
    // emit queueStatusChanged(d->requestQueue.size(), d->activeCount); // Emit later after processing
    QMetaObject::invokeMethod(this, &LazyLoader::processNextRequest, Qt::QueuedConnection);
}

void LazyLoader::queueRequest(const QString& key, ResourceType type, const QVariantMap& params,
                              std::function<void(QVariant)> onSuccess,
                              std::function<void(QString)> onError,
                              qint64 priority)
{
    LoadRequest request;
    request.key = key;
    request.type = type;
    request.parameters = params;
    request.onSuccess = std::move(onSuccess);
    request.onError = std::move(onError);
    request.priority = priority;
    queueRequest(request);
}

bool LazyLoader::cancelRequest(const QString& key)
{
    QMutexLocker locker(&d->mutex);

    // Check active requests first
    if (d->activeRequests.contains(key)) {
        // Cancellation of *active* requests is complex and often not possible
        // without cooperative cancellation points in the loading code itself.
        // For this stub, we'll just log and return false for active requests.
        LOG_WARN("Cannot cancel active request: " << key);
        return false;
    }

    // Remove from queue
    int index = d->findRequestIndex(key);
    if (index >= 0) {
        d->requestQueue.removeAt(index);
        LOG_DEBUG("Canceled queued request: " << key);
        emit queueStatusChanged(d->requestQueue.size(), d->activeCount);
        return true;
    }

    LOG_DEBUG("Request to cancel not found in queue: " << key);
    return false;
}

void LazyLoader::cancelAllRequests()
{
    QMutexLocker locker(&d->mutex);
    int count = d->requestQueue.size();
    d->requestQueue.clear();
    LOG_DEBUG("Canceled all " << count << " queued requests.");
    emit queueStatusChanged(0, d->activeCount);
}

int LazyLoader::queuedRequestCount() const
{
    QMutexLocker locker(&d->mutex);
    return d->requestQueue.size();
}

int LazyLoader::activeRequestCount() const
{
    QMutexLocker locker(&d->mutex);
    return d->activeCount; // Or return d->activeRequests.size(); if they can differ
}

void LazyLoader::setMaxConcurrentLoads(int count)
{
    if (count < 1) return;
    QMutexLocker locker(&d->mutex);
    if (d->maxConcurrent != count) {
        d->maxConcurrent = count;
        LOG_INFO("Max concurrent lazy loads set to " << count);
        // Potentially trigger more processing if limit increased
        QMetaObject::invokeMethod(this, &LazyLoader::processNextRequest, Qt::QueuedConnection);
    }
}

int LazyLoader::maxConcurrentLoads() const
{
    QMutexLocker locker(&d->mutex);
    return d->maxConcurrent;
}

void LazyLoader::preload(const QStringList& resourceKeys, qint64 priority)
{
    LOG_INFO("Preloading " << resourceKeys.size() << " resources with priority " << priority);
    for (const QString& key : resourceKeys) {
        hintResourceNeeded(key); // Bump priority or add a specific preload request
        // A more complex preload might involve creating specific LoadRequest objects
        // for each key with a 'preload' flag or specific parameters.
        // For now, hinting is sufficient.
    }
}

void LazyLoader::hintResourceNeeded(const QString& key)
{
    QMutexLocker locker(&d->mutex);
    // Find the request in the queue and bump its priority
    int index = d->findRequestIndex(key);
    if (index >= 0) {
        d->requestQueue[index].priority += 1000; // Significant bump
        d->requestQueue[index].requestTime = QDateTime::currentDateTime(); // Update time to maintain FIFO for same new priority
        d->sortQueue(); // Re-sort the queue
        LOG_DEBUG("Hinted resource needed, bumped priority: " << key);
    } else {
        // Resource not queued. Could queue a low-priority hint request or just remember the hint.
        // For now, log.
        LOG_DEBUG("Hinted resource not in queue, ignoring: " << key);
        // Could add to a separate 'prediction' list for future proactive loading.
    }
}

void LazyLoader::clear()
{
    cancelAllRequests(); // Clear queue
    // Clearing 'active' requests is not possible without waiting or cancellation.
    // We assume they will finish normally.
    LOG_DEBUG("LazyLoader cleared.");
}

QVariantMap LazyLoader::statistics() const
{
    // This would aggregate stats from ThreadPool, cache hits/misses related to loading, etc.
    QVariantMap stats;
    stats["queuedRequestCount"] = queuedRequestCount();
    stats["activeRequestCount"] = activeRequestCount();
    stats["maxConcurrentLoads"] = maxConcurrentLoads();
    // More stats from ThreadPool or internal tracking could be added
    return stats;
}

void LazyLoader::processNextRequest()
{
    QMutexLocker locker(&d->mutex);

    // Check if we can start another request
    if (d->activeCount >= d->maxConcurrent || d->requestQueue.isEmpty()) {
        // Nothing to do or at concurrency limit
        emit queueStatusChanged(d->requestQueue.size(), d->activeCount);
        return;
    }

    LoadRequest request = d->requestQueue.dequeue();
    d->activeRequests.insert(request.key);
    d->activeCount++;

    LOG_DEBUG("Processing lazy load request: " << request.key << " on thread " << QThread::currentThreadId());

    // Create a Task for the ThreadPool to execute the loading logic
    Task* loadTask = new Task([this, request]() {
        // Simulate loading work
        // In a real implementation, this would contain the actual logic to load
        // the resource based on request.type and request.parameters.
        QVariant resultData;
        QString error;
        bool success = true;

        // --- Simulated Loading Logic ---
        // This is where the actual loading happens (e.g., read file, render page, fetch image).
        // It should respect cancellation if possible.
        QThread::msleep(100 + (rand() % 200)); // Simulate variable work time

        if (request.type == ResourceType::PageThumbnail) {
            // Simulate creating a thumbnail
            // resultData = createThumbnail(request.parameters); // Placeholder
            resultData = QImage(100, 140, QImage::Format_RGB32); // Placeholder image
            if (resultData.value<QImage>().isNull()) {
                error = "Failed to create thumbnail";
                success = false;
            }
        } else if (request.type == ResourceType::PageContent) {
            // Simulate loading page content
            // resultData = loadPageContent(request.parameters); // Placeholder
            resultData = "Simulated page content for " + request.key; // Placeholder string
        } else {
            // Add cases for other resource types
            resultData = "Simulated data for " + request.key; // Generic placeholder
        }
        // --- End Simulated Loading Logic ---

        // Process result on the main thread
        QMetaObject::invokeMethod(this, [this, request, resultData, error, success]() {
             QMutexLocker resultLocker(&d->mutex); // Lock again to update state
             d->activeRequests.remove(request.key);
             d->activeCount--;
             // Remove from any prediction/hint lists if necessary

             if (success && request.onSuccess) {
                 request.onSuccess(resultData);
                 emit resourceLoaded(request.key, resultData);
                 LOG_DEBUG("Successfully loaded resource: " << request.key);
             } else if (!success && request.onError) {
                 request.onError(error);
                 emit resourceLoadFailed(request.key, error);
                 LOG_WARN("Failed to load resource: " << request.key << ", Error: " << error);
             }

             // Process the next request in the queue
             QMetaObject::invokeMethod(this, &LazyLoader::processNextRequest, Qt::QueuedConnection);
        }, Qt::QueuedConnection); // Ensure callback runs on main thread

    }, "LazyLoadTask_" + request.key, Task::Priority::Normal);

    // Submit the task to the global ThreadPool
    ThreadPool::instance().submitTask(loadTask);

    // Update queue status after dequeuing
    emit queueStatusChanged(d->requestQueue.size(), d->activeCount);
}

} // namespace QuantilyxDoc