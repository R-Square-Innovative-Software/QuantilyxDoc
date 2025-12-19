/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_RENDER_THREAD_H
#define QUANTILYX_RENDER_THREAD_H

#include <QThread>
#include <QImage>
#include <QSize>
#include <QRectF>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <memory>

namespace QuantilyxDoc {

class Page;
class Document;

/**
 * @brief A dedicated thread for rendering document pages.
 *
 * Takes rendering requests from the main thread and processes them
 * asynchronously. This prevents the UI from freezing during expensive
 * rendering operations.
 */
class RenderThread : public QThread
{
    Q_OBJECT

public:
    /**
     * @brief Structure holding details for a single rendering request.
     */
    struct RenderRequest {
        Page* page;               // The page to render
        QSize targetSize;         // Target size in pixels
        qreal zoomLevel;          // Zoom level for the render
        int rotation;             // Rotation (0, 90, 180, 270)
        QRectF clipRect;          // Optional clipping rectangle (in page coordinates)
        bool highQuality;         // Whether to use high-quality rendering
        quintptr requestId;       // Unique identifier for the request
        bool canceled;            // Flag set by main thread to cancel request

        RenderRequest(Page* p, const QSize& sz, qreal z, int rot, const QRectF& clip, bool hq, quintptr id)
            : page(p), targetSize(sz), zoomLevel(z), rotation(rot), clipRect(clip), highQuality(hq), requestId(id), canceled(false) {}
    };

    /**
     * @brief Structure holding the result of a rendering request.
     */
    struct RenderResult {
        quintptr requestId;       // ID of the request this result corresponds to
        QImage image;             // The rendered image
        bool success;             // Whether the render was successful
        QString errorMessage;     // Error message if success is false
    };

    /**
     * @brief Constructor.
     * @param parent Optional parent object.
     */
    explicit RenderThread(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     * Stops the thread gracefully.
     */
    ~RenderThread() override;

    /**
     * @brief Submit a rendering request.
     * @param request The render request to submit.
     */
    void submitRequest(const RenderRequest& request);

    /**
     * @brief Cancel a pending rendering request.
     * @param requestId The ID of the request to cancel.
     */
    void cancelRequest(quintptr requestId);

    /**
     * @brief Cancel all pending rendering requests for a specific page.
     * @param page The page whose requests should be canceled.
     */
    void cancelRequestsForPage(Page* page);

    /**
     * @brief Cancel all pending rendering requests.
     */
    void cancelAllRequests();

    /**
     * @brief Check if the thread is currently processing a request.
     * @return True if busy.
     */
    bool isBusy() const;

    /**
     * @brief Get the number of pending requests in the queue.
     * @return Queue size.
     */
    int pendingRequestCount() const;

    /**
     * @brief Get the number of requests currently being processed.
     * @return Processing count (usually 0 or 1 for a single-threaded renderer).
     */
    int activeRequestCount() const;

    /**
     * @brief Set the priority of the rendering thread.
     * @param priority Thread priority.
     */
    void setPriority(Priority priority);

signals:
    /**
     * @brief Emitted when a rendering request is completed.
     * @param result The result of the render.
     */
    void renderCompleted(const QuantilyxDoc::RenderThread::RenderResult& result);

    /**
     * @brief Emitted when the queue status changes.
     * @param pendingCount Number of pending requests.
     * @param activeCount Number of active requests.
     */
    void queueStatusChanged(int pendingCount, int activeCount);

protected:
    /**
     * @brief The main execution point of the thread.
     * Contains the loop that processes requests from the queue.
     */
    void run() override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_RENDER_THREAD_H