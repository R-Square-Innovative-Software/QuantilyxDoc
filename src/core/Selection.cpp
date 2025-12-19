/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "Selection.h"
#include "Document.h"
#include "Page.h"
#include "Clipboard.h" // Assuming a core Clipboard manager exists
#include <QRectF>
#include <QPointF>
#include <QApplication> // For clipboard access if needed
#include <QClipboard>
#include <QMimeData>
#include <QDebug>
#include <algorithm> // For std::find_if, std::sort, std::unique

namespace QuantilyxDoc {

class Selection::Private {
public:
    Private() : document(nullptr) {}
    QList<Segment> segments;
    QPointer<Document> document; // Use QPointer for safety

    // Helper to update internal state based on segments
    void updateState() {
        bool wasEmpty = segments.isEmpty();
        bool wasMultiPage = isMultiPageInternal();
        ContentType oldType = contentTypeInternal();
        QRectF oldBounds = boundingRectInternal();

        // Emit signals if state changed
        if (wasEmpty && !segments.isEmpty()) {
            emit q->cleared(); // Technically opposite, but signal name exists
        }
        if (!wasEmpty && segments.isEmpty()) {
            emit q->cleared();
        }

        if (contentTypeInternal() != oldType) {
            emit q->contentTypeChanged();
        }

        QRectF newBounds = boundingRectInternal();
        if (newBounds != oldBounds) {
            emit q->boundingRectChanged();
        }

        if (isMultiPageInternal() != wasMultiPage) {
            // No specific signal for multipage change, but logic could react
        }

        emit q->changed();
        emit q->canCopyChanged();
        emit q->canCutChanged();
        emit q->canDeleteChanged();
    }

    // Internal helper for contentType
    ContentType contentTypeInternal() const {
        if (segments.isEmpty()) return Text; // Or maybe Mixed? Depends on definition.
        ContentType firstType = segments.first().type;
        bool allSame = std::all_of(segments.begin(), segments.end(),
                                   [firstType](const Segment& s) { return s.type == firstType; });
        return allSame ? firstType : Mixed;
    }

    // Internal helper for boundingRect
    QRectF boundingRectInternal() const {
        if (segments.isEmpty()) return QRectF();
        QRectF combinedRect;
        bool first = true;
        Page* primaryPage = segments.first().page;
        for (const auto& seg : segments) {
            // Assume all segments are transformed to primary page coordinates if multi-page
            // This is a simplification. Real impl needs careful coord handling.
            QRectF adjustedBounds = seg.bounds;
            if (seg.page != primaryPage) {
                 // Transform seg.bounds to primaryPage coordinates if needed
                 // This requires page transformation logic not defined here.
                 // For now, we assume single page or compatible coords.
                 LOG_WARN("Multi-page selection bounding rect calculation requires coordinate transformation.");
            }
            if (first) {
                combinedRect = adjustedBounds;
                first = false;
            } else {
                combinedRect = combinedRect.united(adjustedBounds);
            }
        }
        return combinedRect;
    }

    // Internal helper for isMultiPage
    bool isMultiPageInternal() const {
        if (segments.size() <= 1) return false;
        Page* firstPage = segments.first().page;
        return std::any_of(segments.begin(), segments.end(),
                           [firstPage](const Segment& s) { return s.page != firstPage; });
    }

    // Pointer to parent Selection object to emit signals
    Selection* q;
};

Selection::Selection(QObject* parent)
    : QObject(parent)
    , d(new Private())
{
    d->q = this; // Set back-pointer for signal emission in private helpers
}

Selection::~Selection()
{
    clear(); // Ensure segments are cleared properly
}

void Selection::clear()
{
    d->segments.clear();
    d->updateState();
}

bool Selection::isEmpty() const
{
    return d->segments.isEmpty();
}

QList<Selection::Segment> Selection::segments() const
{
    return d->segments;
}

Selection::Segment Selection::primarySegment() const
{
    if (!d->segments.isEmpty()) {
        return d->segments.first();
    }
    return Segment(); // Return default constructed segment
}

Document* Selection::document() const
{
    return d->document.data();
}

void Selection::setDocument(Document* doc)
{
    d->document = doc; // QPointer handles deletion
}

Selection::ContentType Selection::contentType() const
{
    return d->contentTypeInternal();
}

QRectF Selection::boundingRect() const
{
    return d->boundingRectInternal();
}

bool Selection::isMultiPage() const
{
    return d->isMultiPageInternal();
}

QList<Page*> Selection::pages() const
{
    QList<Page*> pageList;
    pageList.reserve(d->segments.size());
    for (const auto& seg : d->segments) {
        if (!pageList.contains(seg.page)) {
            pageList.append(seg.page);
        }
    }
    return pageList;
}

QString Selection::selectedText() const
{
    if (contentType() != Text && contentType() != Mixed) {
        return QString(); // Only return text if primarily text content
    }
    QStringList texts;
    for (const auto& seg : d->segments) {
        if (seg.type == Text || seg.type == Mixed) {
            texts << seg.content.toString(); // Assumes content is stored as QString for Text type
        }
    }
    return texts.join("\n---\n"); // Join multi-segment text with a separator
}

bool Selection::selectRegion(Page* page, const QRectF& region, ContentType typeHint)
{
    if (!page || region.isEmpty()) return false;

    // This is a simplified stub. In reality, the Page class would need a method
    // like findContent(region, typeHint) that returns the actual content and its bounds.
    // For now, we'll create a dummy segment representing the region.
    Segment seg;
    seg.page = page;
    seg.bounds = region;
    seg.type = typeHint; // Use hint or try to determine from page
    seg.content = QString("Selected region on page %1").arg(page->pageIndex()); // Dummy content
    seg.context = QString("Context for region %1").arg(region.toString()); // Dummy context
    seg.startIndex = -1; // Unknown
    seg.endIndex = -1;   // Unknown

    d->segments.clear(); // Simple implementation: replace current selection
    d->segments.append(seg);
    d->updateState();
    return true;
}

bool Selection::extendSelection(Page* page, const QRectF& region)
{
    // Similar to selectRegion, but adds to existing selection
    if (!page || region.isEmpty()) return false;

    Segment seg;
    seg.page = page;
    seg.bounds = region;
    seg.type = Text; // Default assumption
    seg.content = QString("Extended selection on page %1").arg(page->pageIndex());
    seg.context = QString("Extended context for region %1").arg(region.toString());
    seg.startIndex = -1;
    seg.endIndex = -1;

    d->segments.append(seg); // Add to existing list
    d->updateState();
    return true;
}

bool Selection::selectObject(Page* page, QObject* object)
{
    if (!page || !object) return false;

    // This is a stub. The object would need to provide its bounds and type.
    // For example, an Annotation object might inherit from a selectable base class.
    // Assume for now we can get bounds and type from the object via some interface.
    // This requires a more complex object model than shown here.
    // For now, return false indicating it's not implemented.
    Q_UNUSED(page);
    Q_UNUSED(object);
    LOG_WARN("selectObject not fully implemented.");
    return false;
}

bool Selection::selectPage(Page* page)
{
    if (!page) return false;

    // Select the entire page content area
    QRectF pageBounds(QPointF(0, 0), page->size());
    return selectRegion(page, pageBounds, PageElement);
}

int Selection::count() const
{
    return d->segments.size();
}

bool Selection::copyToClipboard() const
{
    if (isEmpty() || !canCopy()) return false;

    QClipboard* clipboard = QApplication::clipboard();
    if (!clipboard) return false;

    QMimeData* mimeData = new QMimeData();

    ContentType type = contentType();
    if (type == Text || type == Mixed) {
        mimeData->setText(selectedText());
        mimeData->setHtml(selectedText()); // Basic HTML representation
    }
    // Add other formats based on content type (images, etc.)

    clipboard->setMimeData(mimeData);
    LOG_INFO("Copied selection to clipboard.");
    return true;
}

bool Selection::cutToClipboard()
{
    if (isEmpty() || !canCut()) return false;

    bool copied = copyToClipboard();
    if (copied) {
        // After copying, delete the original content
        bool deleted = deleteContent();
        return deleted;
    }
    return false;
}

bool Selection::deleteContent()
{
    if (isEmpty() || !canDelete()) return false;

    // This is a stub. Deletion requires modifying the underlying Document/Page content.
    // The Segment structure would need to contain enough information to perform the deletion.
    // This is highly dependent on the specific Document/Page implementation.
    LOG_WARN("deleteContent not fully implemented. Requires document modification logic.");
    // Example: For each segment, call page->removeContent(startIndex, endIndex) or similar.
    // This would require the Page/Document classes to expose such methods.
    clear(); // For demo purposes, just clear the selection
    return true;
}

bool Selection::canCopy() const
{
    return !isEmpty() && contentType() != Mixed; // Simplified rule
}

bool Selection::canCut() const
{
    return canCopy() && !isMultiPage(); // Cannot cut multi-page content easily
}

bool Selection::canDelete() const
{
    return !isEmpty(); // Assuming anything selected can be deleted
}

} // namespace QuantilyxDoc