/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "StatusBar.h"
#include "../core/Document.h"
#include "../core/Logger.h"
#include <QLabel>
#include <QProgressBar>
#include <QSpinBox>
#include <QSlider>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QApplication>
#include <QDebug>

namespace QuantilyxDoc {

class StatusBar::Private {
public:
    Private(StatusBar* q_ptr)
        : q(q_ptr), document(nullptr), currentPageIndex(-1), zoomLevelVal(1.0), rotationVal(0), progressVal(-1) {}

    StatusBar* q;
    QPointer<Document> document; // Use QPointer for safety

    // Status bar widgets
    QLabel* messageLabel; // For temporary messages (handled by base QStatusBar)
    QLabel* pageLabel; // Static "Page:" label
    QSpinBox* pageSpinBox; // For selecting page
    QLabel* pageCountLabel; // Displays total pages (e.g., "/ 10")
    QLabel* zoomLabel; // Static "Zoom:" label
    QSlider* zoomSlider; // For adjusting zoom (10% to 500%)
    QLabel* zoomPercentLabel; // Displays current zoom as percentage (e.g., "100%")
    QLabel* rotationLabel; // Static "Rotation:" label
    QToolButton* rotateLeftButton; // Rotate -90 degrees
    QToolButton* rotateRightButton; // Rotate +90 degrees
    QLabel* rotationValueLabel; // Displays current rotation (e.g., "0°")
    QProgressBar* progressBar; // For showing operation progress
    QLabel* statusTextLabel; // For permanent status text (e.g., "Ready", "Rendering...")

    int currentPageIndex;
    qreal zoomLevelVal;
    int rotationVal;
    int progressVal; // -1 means hidden

    // Helper to setup the custom widgets and add them to the status bar
    void setupCustomWidgets() {
        // --- Permanent Status Text ---
        statusTextLabel = new QLabel(tr("Ready"), q);
        statusTextLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        statusTextLabel->setMinimumWidth(100); // Ensure some space for status text
        q->addWidget(statusTextLabel); // Add to left side

        // --- Page Controls ---
        pageLabel = new QLabel(tr("Page:"), q);
        pageLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        pageLabel->setMinimumWidth(40); // Ensure consistent width

        pageSpinBox = new QSpinBox(q);
        pageSpinBox->setRange(1, 1); // Will be updated by document
        pageSpinBox->setValue(1);
        pageSpinBox->setMinimumWidth(60); // Ensure consistent width
        pageSpinBox->setAlignment(Qt::AlignCenter);

        pageCountLabel = new QLabel(tr("/ 1"), q); // Will be updated by document
        pageCountLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        pageCountLabel->setMinimumWidth(40); // Ensure consistent width

        // Add page widgets as permanent items on the right
        q->addPermanentWidget(pageLabel);
        q->addPermanentWidget(pageSpinBox);
        q->addPermanentWidget(pageCountLabel);

        // --- Zoom Controls ---
        zoomLabel = new QLabel(tr("Zoom:"), q);
        zoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        zoomLabel->setMinimumWidth(40);

        zoomSlider = new QSlider(Qt::Horizontal, q);
        zoomSlider->setRange(10, 500); // 10% to 500%
        zoomSlider->setValue(100); // Default 100%
        zoomSlider->setTickPosition(QSlider::TicksBelow);
        zoomSlider->setTickInterval(50);
        zoomSlider->setMinimumWidth(100); // Ensure consistent width

        zoomPercentLabel = new QLabel(tr("100%"), q);
        zoomPercentLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        zoomPercentLabel->setMinimumWidth(40);

        // Add zoom widgets as permanent items on the right
        q->addPermanentWidget(zoomLabel);
        q->addPermanentWidget(zoomSlider);
        q->addPermanentWidget(zoomPercentLabel);

        // --- Rotation Controls ---
        rotationLabel = new QLabel(tr("Rotation:"), q);
        rotationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        rotationLabel->setMinimumWidth(60);

        rotateLeftButton = new QToolButton(q);
        rotateLeftButton->setText(tr("↺")); // Unicode for counter-clockwise arrow
        rotateLeftButton->setToolTip(tr("Rotate Left (90° CCW)"));
        rotateLeftButton->setAutoRaise(true);

        rotateRightButton = new QToolButton(q);
        rotateRightButton->setText(tr("↻")); // Unicode for clockwise arrow
        rotateRightButton->setToolTip(tr("Rotate Right (90° CW)"));
        rotateRightButton->setAutoRaise(true);

        rotationValueLabel = new QLabel(tr("0°"), q);
        rotationValueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        rotationValueLabel->setMinimumWidth(30);

        // Add rotation widgets as permanent items on the right
        q->addPermanentWidget(rotationLabel);
        q->addPermanentWidget(rotateLeftButton);
        q->addPermanentWidget(rotateRightButton);
        q->addPermanentWidget(rotationValueLabel);

        // --- Progress Bar (initially hidden) ---
        progressBar = new QProgressBar(q);
        progressBar->setVisible(false); // Hidden by default
        progressBar->setMaximumWidth(150); // Limit width
        progressBar->setTextVisible(true);
        // Add progress bar as a permanent item, likely near the right
        q->addPermanentWidget(progressBar);

        // Connect signals
        QObject::connect(pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                         q, [this](int value) { emit q->pageChanged(value - 1); }); // Emit 0-based index

        QObject::connect(zoomSlider, &QSlider::valueChanged,
                         q, [this](int value) {
                             qreal newZoom = value / 100.0;
                             setZoomLevel(newZoom);
                             emit q->zoomLevelChanged(newZoom);
                         });

        QObject::connect(rotateLeftButton, &QToolButton::clicked,
                         q, [this]() {
                             int newRotation = (d->rotationVal - 90 + 360) % 360;
                             setRotation(newRotation);
                             emit q->rotationChanged(newRotation);
                         });

        QObject::connect(rotateRightButton, &QToolButton::clicked,
                         q, [this]() {
                             int newRotation = (d->rotationVal + 90) % 360;
                             setRotation(newRotation);
                             emit q->rotationChanged(newRotation);
                         });

        LOG_DEBUG("StatusBar: Custom widgets initialized.");
    }

    // Helper to update the page count label text
    void updatePageCountLabel() {
        if (document) {
            pageCountLabel->setText(tr("/ %1").arg(document->pageCount()));
        } else {
            pageCountLabel->setText(tr("/ 0"));
        }
    }

    // Helper to update the zoom percentage label text and slider value
    void updateZoomLabel() {
        int percent = qRound(zoomLevelVal * 100);
        zoomPercentLabel->setText(tr("%1%").arg(percent));
        zoomSlider->setValue(percent); // Sync slider to value
    }

    // Helper to update the rotation value label text and button states if needed
    void updateRotationLabel() {
        rotationValueLabel->setText(tr("%1°").arg(rotationVal));
        // Could enable/disable buttons based on current rotation if desired
        // rotateLeftButton->setEnabled(...);
        // rotateRightButton->setEnabled(...);
    }

    // Helper to set zoom level (internal, updates slider and label)
    void setZoomLevel(qreal zoom) {
        if (zoom > 0) {
            zoomLevelVal = zoom;
            updateZoomLabel();
        }
    }

    // Helper to set rotation (internal, updates label)
    void setRotation(int degrees) {
        if (degrees % 90 == 0) { // Only allow 90-degree increments
            int normalizedDegrees = ((degrees % 360) + 360) % 360; // Normalize to 0-359
            rotationVal = normalizedDegrees;
            updateRotationLabel();
        }
    }
};

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent)
    , d(new Private(this))
{
    d->setupCustomWidgets();
    setDocument(nullptr); // Initialize with no document

    LOG_INFO("StatusBar initialized.");
}

StatusBar::~StatusBar()
{
    LOG_INFO("StatusBar destroyed.");
}

void StatusBar::setDocument(Document* doc)
{
    if (d->document == doc) return;

    // Disconnect from old document signals if necessary
    if (d->document) {
        // disconnect(d->document, &Document::currentPageChanged, ...);
        // disconnect(d->document, &Document::pageCountChanged, ...);
    }

    d->document = doc; // Use QPointer

    if (doc) {
        // Connect to new document signals
        // connect(doc, &Document::currentPageChanged, this, [this](int index) {
        //     setCurrentPage(index);
        // });
        // connect(doc, &Document::pageCountChanged, this, [this]() {
        //     d->updatePageCountLabel();
        // });

        // Update UI based on new document state
        d->pageSpinBox->setRange(1, doc->pageCount());
        d->pageSpinBox->setValue(doc->currentPageIndex() + 1); // 0-based to 1-based
        d->updatePageCountLabel();
        showMessage(tr("Loaded: %1").arg(doc->filePath()), 3000); // Show for 3 seconds
    } else {
        d->pageSpinBox->setRange(1, 1);
        d->pageSpinBox->setValue(1);
        d->updatePageCountLabel();
        showMessage(tr("Ready"), 2000);
    }

    LOG_DEBUG("StatusBar set to document: " << (doc ? doc->filePath() : "nullptr"));
}

Document* StatusBar::document() const
{
    return d->document.data(); // Returns nullptr if document was deleted
}

void StatusBar::setCurrentPage(int index)
{
    if (index >= 0) {
        int oldPage = d->currentPageIndex;
        d->currentPageIndex = index;
        d->pageSpinBox->setValue(index + 1); // Update spinbox (1-based)

        if (oldPage != index) {
            // Signal is emitted by the spinbox's valueChanged signal connection
            LOG_DEBUG("StatusBar current page updated to " << index);
        }
    }
}

int StatusBar::currentPage() const
{
    return d->currentPageIndex;
}

void StatusBar::setZoomLevel(qreal zoom)
{
    if (zoom > 0) {
        qreal oldZoom = d->zoomLevelVal;
        d->setZoomLevel(zoom); // Use private helper

        if (!qFuzzyCompare(oldZoom, zoom)) {
            // Signal is emitted by the slider's valueChanged signal connection
            LOG_DEBUG("StatusBar zoom level updated to " << zoom);
        }
    }
}

qreal StatusBar::zoomLevel() const
{
    return d->zoomLevelVal;
}

void StatusBar::setRotation(int degrees)
{
    if (degrees % 90 == 0) { // Only allow 90-degree increments
        int oldRotation = d->rotationVal;
        d->setRotation(degrees); // Use private helper

        if (oldRotation != degrees) {
            // Signal is emitted by the button's clicked signal connection
            LOG_DEBUG("StatusBar rotation updated to " << degrees);
        }
    }
}

int StatusBar::rotation() const
{
    return d->rotationVal;
}

void StatusBar::showMessage(const QString& message, int timeoutMs)
{
    QStatusBar::showMessage(message, timeoutMs);
    // Update the permanent status text label too, maybe briefly?
    // A more sophisticated approach might only update statusTextLabel for permanent statuses.
    LOG_DEBUG("StatusBar message: " << message << " (timeout: " << timeoutMs << "ms)");
}

void StatusBar::clearMessage()
{
    QStatusBar::clearMessage();
    LOG_DEBUG("StatusBar message cleared.");
}

void StatusBar::setProgress(int value)
{
    if (value >= 0 && value <= 100) {
        d->progressVal = value;
        d->progressBar->setValue(value);
        bool wasVisible = d->progressBar->isVisible();
        bool shouldBeVisible = value < 100; // Hide when at 100% (operation finished) or explicitly set to -1
        if (wasVisible != shouldBeVisible) {
            d->progressBar->setVisible(shouldBeVisible);
        }
        if (value < 100) {
             emit operationStarted(); // Or use a dedicated progress signal?
        } else {
             emit operationFinished();
        }
        LOG_DEBUG("StatusBar progress set to " << value << "%");
    } else if (value < 0) {
        // Negative value hides the progress bar
        d->progressVal = -1;
        d->progressBar->setVisible(false);
        emit operationFinished(); // Implies operation ended/cancelled
        LOG_DEBUG("StatusBar progress bar hidden.");
    }
}

int StatusBar::progress() const
{
    return d->progressVal;
}

void StatusBar::setProgressVisible(bool visible)
{
    d->progressBar->setVisible(visible);
    if (!visible) d->progressVal = -1; // Reset internal state if hidden
    LOG_DEBUG("StatusBar progress bar set to " << (visible ? "visible" : "hidden"));
}

void StatusBar::setPageControlsVisible(bool visible)
{
    d->pageLabel->setVisible(visible);
    d->pageSpinBox->setVisible(visible);
    d->pageCountLabel->setVisible(visible);
    LOG_DEBUG("StatusBar page controls set to " << (visible ? "visible" : "hidden"));
}

void StatusBar::setZoomControlsVisible(bool visible)
{
    d->zoomLabel->setVisible(visible);
    d->zoomSlider->setVisible(visible);
    d->zoomPercentLabel->setVisible(visible);
    LOG_DEBUG("StatusBar zoom controls set to " << (visible ? "visible" : "hidden"));
}

void StatusBar::setRotationControlsVisible(bool visible)
{
    d->rotationLabel->setVisible(visible);
    d->rotateLeftButton->setVisible(visible);
    d->rotateRightButton->setVisible(visible);
    d->rotationValueLabel->setVisible(visible);
    LOG_DEBUG("StatusBar rotation controls set to " << (visible ? "visible" : "hidden"));
}

QString StatusBar::currentDocumentPath() const
{
    return d->document ? d->document->filePath() : QString();
}

int StatusBar::currentPageCount() const
{
    return d->document ? d->document->pageCount() : 0;
}

QString StatusBar::documentStatus() const
{
    // This could reflect the state of the current document (e.g., "Loading", "Rendering", "Ready", "Modified")
    // It might be updated by connecting to Document signals.
    if (d->document) {
        // Example: Check if document is loading, rendering, etc.
        // This requires signals from Document/DocumentView/RenderThread
        // For now, use a placeholder or the permanent status label's text
        return d->statusTextLabel->text();
    }
    return "No Document";
}

void StatusBar::setDocumentStatus(const QString& status)
{
    // Updates the permanent status text label
    d->statusTextLabel->setText(status);
    LOG_DEBUG("StatusBar document status set to: " << status);
}

} // namespace QuantilyxDoc