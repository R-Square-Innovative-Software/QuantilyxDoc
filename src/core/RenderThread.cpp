/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "RenderThread.h"
#include "Page.h"
#include "Document.h"
#include "Logger.h"
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QQueue>
#include <QImage>
#include <QPainter>
#include <QThread>
#include <QDebug>

namespace QuantilyxDoc {

class RenderThread::Private {
public:
    Private(RenderThread* q_ptr) : q(q_ptr), shouldQuit(false), isProcessing(false) {}

    RenderThread* q;
    mutable QMutex mutex;
    QWaitCondition condition;
    QQueue<RenderRequest> requestQueue;
    QSet<quintptr> activeRequestIds; // IDs currently being processed
    volatile bool shouldQuit;
    bool isProcessing; // Flag to indicate if a request is actively being worked on

    // Helper to process a single request
    RenderResult processRequest(const RenderRequest& req) {
        RenderResult result;
        result.requestId = req.requestId;
        result.success = false;

        if (req.canceled) {
            result.errorMessage = "Request was canceled.";
            LOG_DEBUG("Render request " << req.requestId << " was canceled before processing.");
            return result;
        }

        if (!req.page) {
            result.errorMessage = "Invalid page pointer.";
            LOG_ERROR("Render request " << req.requestId << " has null page pointer.");
            return result;
        }

        // --- Simulated Rendering Logic ---
        // In a real implementation, this would call page->render(targetSize, zoom, rotation, clipRect, highQuality)
        // Here, we create a placeholder image based on page size and request parameters.
        QSizeF pageSize = req.page->size(); // Size in points
        if (pageSize.isEmpty()) {
            result.errorMessage = "Page has invalid size.";
            LOG_ERROR("Page " << req.page->pageIndex() << " has invalid size for render request " << req.requestId);
            return result;
        }

        // Calculate scaling factors to convert from points to pixels
        qreal dpi = 72.0; // Base DPI
        qreal scaleX = (req.targetSize.width() > 0) ? (req.targetSize.width() / pageSize.width()) * 72.0 / dpi : 1.0;
        qreal scaleY = (req.targetSize.height() > 0) ? (req.targetSize.height() / pageSize.height()) * 72.0 / dpi : 1.0;
        // Use average scale or take min/max depending on fit strategy (FitWidth, FitHeight, FitBoth)
        qreal scale = qMin(scaleX, scaleY);

        QSize renderSize = QSize(qRound(pageSize.width() * scale), qRound(pageSize.height() * scale));

        // Create image
        QImage image(renderSize, QImage::Format_ARGB32_Premultiplied);
        if (image.isNull()) {
            result.errorMessage = "Failed to create image buffer.";
            LOG_ERROR("Failed to create image buffer for render request " << req.requestId);
            return result;
        }
        image.fill(Qt::lightGray); // Placeholder background

        QPainter painter(&image);
        if (!painter.isActive()) {
            result.errorMessage = "Failed to initialize painter.";
            LOG_ERROR("Failed to initialize painter for render request " << req.requestId);
            return result;
        }

        // Apply transformations based on rotation
        switch (req.rotation) {
            case 90:
                painter.translate(image.width(), 0);
                painter.rotate(90);
                break;
            case 180:
                painter.translate(image.width(), image.height());
                painter.rotate(180);
                break;
            case 270:
                painter.translate(0, image.height());
                painter.rotate(270);
                break;
            default:
                break; // No rotation
        }

        // Simulate drawing page content (placeholder)
        painter.fillRect(0, 0, pageSize.width() * scale, pageSize.height() * scale, QColor(200, 220, 255)); // Light blue page
        painter.setPen(Qt::black);
        painter.drawText(QRectF(10, 10, pageSize.width() * scale - 20, 20), Qt::AlignLeft, QString("Page %1").arg(req.page->pageIndex()));

        painter.end();

        result.image = image;
        result.success = true;
        LOG_DEBUG("Successfully rendered page " << req.page->pageIndex() << " for request " << req.requestId);

        return result;
    }
};

RenderThread::RenderThread(QObject* parent)
    : QThread(parent)
    , d(new Private(this))
{
    // Start the thread upon construction
    start(HighestPriority); // Rendering should be fast, but prioritize responsiveness
}

RenderThread::~RenderThread()
{
    d->shouldQuit = true;
    d->condition.wakeAll(); // Wake up the thread if it's waiting
    wait(); // Wait for the thread to finish its current task and exit
    // Mutex lock is implicit in member functions accessing d->requestQueue
    QMutexLocker locker(&d->mutex);
    // Clear any remaining requests
    while (!d->requestQueue.isEmpty()) {
        auto req = d->requestQueue.dequeue();
        LOG_WARN("Discarding render request " << req.requestId << " during shutdown.");
    }
}

void RenderThread::submitRequest(const RenderRequest& request)
{
    QMutexLocker locker(&d->mutex);
    d->requestQueue.enqueue(request);
    d->condition.wakeOne(); // Notify the thread that a new request is available
    emit queueStatusChanged(d->requestQueue.size(), d->activeRequestIds.size());
}

void RenderThread::cancelRequest(quintptr requestId)
{
    QMutexLocker locker(&d->mutex);
    // Mark the specific request as canceled if it's queued
    auto it = std::find_if(d->requestQueue.begin(), d->requestQueue.end(),
                           [requestId](const RenderRequest& r) { return r.requestId == requestId; });
    if (it != d->requestQueue.end()) {
        it->canceled = true;
        LOG_DEBUG("Marked render request " << requestId << " as canceled (queued).");
    }
    // If the request is already active, the processing logic in run() will check the flag.
    // Adding the ID to a separate 'cancelledActive' set could make this more efficient if needed.
    // For now, the active request processing loop checks the flag inside the request object itself.
}

void RenderThread::cancelRequestsForPage(Page* page)
{
    QMutexLocker locker(&d->mutex);
    if (!page) return;

    // Mark all queued requests for this page as canceled
    for (auto& req : d->requestQueue) {
        if (req.page == page) {
            req.canceled = true;
            LOG_DEBUG("Marked render request " << req.requestId << " for page " << page->pageIndex() << " as canceled (queued).");
        }
    }
    // Active requests for this page will be checked similarly during processing.
}

void RenderThread::cancelAllRequests()
{
    QMutexLocker locker(&d->mutex);
    // Mark all queued requests as canceled
    for (auto& req : d->requestQueue) {
        req.canceled = true;
    }
    LOG_DEBUG("Marked all " << d->requestQueue.size() << " queued render requests as canceled.");
    // Active requests will also be checked.
}

bool RenderThread::isBusy() const
{
    QMutexLocker locker(&d->mutex);
    return d->isProcessing || !d->requestQueue.isEmpty();
}

int RenderThread::pendingRequestCount() const
{
    QMutexLocker locker(&d->mutex);
    return d->requestQueue.size();
}

int RenderThread::activeRequestCount() const
{
    QMutexLocker locker(&d->mutex);
    return d->activeRequestIds.size();
}

void RenderThread::setPriority(Priority priority)
{
    QThread::setPriority(priority);
}

void RenderThread::run()
{
    forever {
        RenderRequest request;
        {
            QMutexLocker locker(&d->mutex);
            // Wait until there's work to do or we need to quit
            while (d->requestQueue.isEmpty() && !d->shouldQuit) {
                d->condition.wait(&d->mutex);
            }

            if (d->shouldQuit) {
                break; // Exit the loop and terminate the thread
            }

            // Dequeue the next request
            request = d->requestQueue.dequeue();
            d->activeRequestIds.insert(request.requestId);
            d->isProcessing = true;
        }

        // Process the request *outside* the mutex lock to avoid blocking other threads submitting requests
        RenderResult result = processRequest(request);

        // Acquire lock again to emit signal and update state
        {
            QMutexLocker locker(&d->mutex);
            d->activeRequestIds.remove(request.requestId);
            d->isProcessing = false;
            emit queueStatusChanged(d->requestQueue.size(), d->activeRequestIds.size());
        }

        // Emit the result on the thread where this object lives (usually main thread due to queued connections)
        emit renderCompleted(result);

    } // forever
    LOG_DEBUG("RenderThread " << QThread::currentThreadId() << " exiting run loop.");
}

} // namespace QuantilyxDoc