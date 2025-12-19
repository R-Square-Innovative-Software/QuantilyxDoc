/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_SELECTION_H
#define QUANTILYX_SELECTION_H

#include <QObject>
#include <QRectF>
#include <QPointF>
#include <QList>
#include <QVariant>
#include <memory>

namespace QuantilyxDoc {

class Page;
class Document;

/**
 * @brief Represents a selection within a document.
 *
 * Can represent selections of text, images, annotations, or other objects
 * across one or multiple pages. Provides methods to manipulate and query
 * the selected content.
 */
class Selection : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Type of content selected.
     */
    enum ContentType {
        Text,
        Image,
        Annotation,
        Link,
        PageElement, // Generic page element
        Mixed        // Multiple types within selection
    };

    /**
     * @brief A single segment of the selection.
     * For complex selections (e.g., multi-page text), the overall selection
     * might consist of multiple segments.
     */
    struct Segment {
        Page* page;           // Page containing this segment
        QRectF bounds;        // Bounds of the segment in page coordinates
        ContentType type;     // Type of content selected
        QVariant content;     // Actual content data (e.g., QString for text)
        QString context;      // Surrounding context for verification
        int startIndex;       // Index within the page's content (e.g., character index)
        int endIndex;         // End index within the page's content
    };

    /**
     * @brief Constructor.
     * @param parent Parent object (usually the Document or DocumentView).
     */
    explicit Selection(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~Selection() override;

    /**
     * @brief Clear the current selection.
     */
    void clear();

    /**
     * @brief Check if there is an active selection.
     * @return True if something is selected.
     */
    bool isEmpty() const;

    /**
     * @brief Get the list of selected segments.
     * @return List of Segment structures.
     */
    QList<Segment> segments() const;

    /**
     * @brief Get the primary selected segment (first one).
     * @return Primary segment, or an invalid one if empty.
     */
    Segment primarySegment() const;

    /**
     * @brief Get the document this selection belongs to.
     * @return Pointer to the document.
     */
    Document* document() const;

    /**
     * @brief Set the document this selection belongs to.
     * @param doc The document.
     */
    void setDocument(Document* doc);

    /**
     * @brief Get the content type of the selection.
     * @return ContentType enum value. Returns Mixed if multiple types are selected.
     */
    ContentType contentType() const;

    /**
     * @brief Get the bounding rectangle of the entire selection.
     * @return Bounding rectangle encompassing all selected segments, or null rect if empty.
     * The rectangle is in the coordinate system of the primary page.
     */
    QRectF boundingRect() const;

    /**
     * @brief Check if the selection spans multiple pages.
     * @return True if segments belong to different pages.
     */
    bool isMultiPage() const;

    /**
     * @brief Get the list of unique pages involved in the selection.
     * @return List of Page pointers.
     */
    QList<Page*> pages() const;

    /**
     * @brief Get the selected text content.
     * Only applicable if contentType() is Text or Mixed.
     * @return Selected text string.
     */
    QString selectedText() const;

    /**
     * @brief Select content based on a region on a page.
     * @param page The page to select on.
     * @param region The region in page coordinates to select.
     * @param typeHint Hint about the expected content type (can be used for refinement).
     * @return True if content was successfully selected.
     */
    bool selectRegion(Page* page, const QRectF& region, ContentType typeHint = Text);

    /**
     * @brief Extend the current selection to include a new region.
     * @param page The page containing the new region.
     * @param region The new region in page coordinates.
     * @return True if the selection was extended.
     */
    bool extendSelection(Page* page, const QRectF& region);

    /**
     * @brief Select a specific object (e.g., annotation, link).
     * @param page The page containing the object.
     * @param object Pointer to the object to select.
     * @return True if the object was successfully selected.
     */
    bool selectObject(Page* page, QObject* object);

    /**
     * @brief Select an entire page.
     * @param page The page to select.
     * @return True if the page was selected.
     */
    bool selectPage(Page* page);

    /**
     * @brief Get the number of segments in the selection.
     * @return Number of segments.
     */
    int count() const;

    /**
     * @brief Copy the selected content to the clipboard.
     * @return True if the copy operation was successful.
     */
    bool copyToClipboard() const;

    /**
     * @brief Cut the selected content to the clipboard.
     * This removes the content from the document.
     * @return True if the cut operation was successful.
     */
    bool cutToClipboard();

    /**
     * @brief Delete the selected content from the document.
     * @return True if the deletion was successful.
     */
    bool deleteContent();

    /**
     * @brief Check if the selection can be copied.
     * @return True if copy is allowed.
     */
    bool canCopy() const;

    /**
     * @brief Check if the selection can be cut.
     * @return True if cut is allowed.
     */
    bool canCut() const;

    /**
     * @brief Check if the selection can be deleted.
     * @return True if deletion is allowed.
     */
    bool canDelete() const;

signals:
    /**
     * @brief Emitted when the selection changes (added, removed, modified).
     */
    void changed();

    /**
     * @brief Emitted when the selection becomes empty.
     */
    void cleared();

    /**
     * @brief Emitted when the content type of the selection changes.
     */
    void contentTypeChanged();

    /**
     * @brief Emitted when the bounding rectangle changes.
     */
    void boundingRectChanged();

    /**
     * @brief Emitted when the ability to perform actions changes.
     */
    void canCopyChanged();
    void canCutChanged();
    void canDeleteChanged();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_SELECTION_H