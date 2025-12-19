/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_PAGE_H
#define QUANTILYX_PAGE_H

#include <QObject>
#include <QSizeF>
#include <QRectF>
#include <QImage>
#include <QList>
#include <memory>

namespace QuantilyxDoc {

class Document;
class Annotation;
class PageContent;
class PageRenderer;

/**
 * @brief Page rotation enumeration
 */
enum class PageRotation
{
    Degrees0 = 0,
    Degrees90 = 90,
    Degrees180 = 180,
    Degrees270 = 270
};

/**
 * @brief Page layout mode
 */
enum class PageLayout
{
    SinglePage,     ///< Single page layout
    FacingPages,    ///< Two pages facing each other
    BookView        ///< Book view with facing pages and odd pages on right
};

/**
 * @brief Base page class
 * 
 * Abstract base class for all document pages. Provides common interface
 * for page operations like rendering, hit testing, and content access.
 */
class Page : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Destructor
     */
    virtual ~Page();
    
    /**
     * @brief Get page number (1-based)
     * @return Page number
     */
    int pageNumber() const;
    
    /**
     * @brief Get page index (0-based)
     * @return Page index
     */
    int pageIndex() const;
    
    /**
     * @brief Get page size in points (1/72 inch)
     * @return Page size
     */
    QSizeF size() const;
    
    /**
     * @brief Get page rotation
     * @return Page rotation
     */
    PageRotation rotation() const;
    
    /**
     * @brief Get page label (custom page number)
     * @return Page label or empty string if none
     */
    QString label() const;
    
    /**
     * @brief Get page title
     * @return Page title or empty string if none
     */
    QString title() const;
    
    /**
     * @brief Check if page is visible
     * @return true if visible
     */
    bool isVisible() const;
    
    /**
     * @brief Get page content box
     * @return Content box in page coordinates
     */
    QRectF contentBox() const;
    
    /**
     * @brief Get page annotations
     * @return List of annotations
     */
    virtual QList<Annotation*> annotations() const;
    
    /**
     * @brief Add annotation to page
     * @param annotation Annotation to add
     */
    virtual void addAnnotation(Annotation* annotation);
    
    /**
     * @brief Remove annotation from page
     * @param annotation Annotation to remove
     */
    virtual void removeAnnotation(Annotation* annotation);
    
    /**
     * @brief Get page content
     * @return Page content object
     */
    virtual PageContent* content() const;
    
    /**
     * @brief Get page renderer
     * @return Page renderer
     */
    virtual PageRenderer* renderer() const;
    
    /**
     * @brief Render page to image
     * @param width Target width in pixels
     * @param height Target height in pixels
     * @param dpi DPI for rendering
     * @return Rendered image
     */
    virtual QImage render(int width, int height, int dpi = 72) = 0;
    
    /**
     * @brief Get text content of page
     * @return Text content
     */
    virtual QString text() const;
    
    /**
     * @brief Search for text on page
     * @param text Text to search
     * @param caseSensitive Case sensitive search
     * @param wholeWords Whole words only
     * @return List of text rectangles where text was found
     */
    virtual QList<QRectF> searchText(const QString& text, 
                                   bool caseSensitive = false,
                                   bool wholeWords = false) const;
    
    /**
     * @brief Hit test - find object at position
     * @param position Position in page coordinates
     * @return Object at position or nullptr
     */
    virtual QObject* hitTest(const QPointF& position) const;
    
    /**
     * @brief Get page links
     * @return List of links on page
     */
    virtual QList<QObject*> links() const;
    
    /**
     * @brief Get page metadata
     * @return Metadata as QVariantMap
     */
    virtual QVariantMap metadata() const;

signals:
    /**
     * @brief Emitted when page content changes
     */
    void contentChanged();
    
    /**
     * @brief Emitted when page annotations change
     */
    void annotationsChanged();
    
    /**
     * @brief Emitted when page is rendered
     */
    void rendered();

protected:
    /**
     * @brief Protected constructor
     * @param document Parent document
     * @param parent Parent object
     */
    explicit Page(Document* document, QObject* parent = nullptr);
    
    /**
     * @brief Set page number
     * @param number Page number (1-based)
     */
    void setPageNumber(int number);
    
    /**
     * @brief Set page size
     * @param size Page size in points
     */
    void setSize(const QSizeF& size);
    
    /**
     * @brief Set page rotation
     * @param rotation Page rotation
     */
    void setRotation(PageRotation rotation);
    
    /**
     * @brief Set page label
     * @param label Page label
     */
    void setLabel(const QString& label);
    
    /**
     * @brief Set page title
     * @param title Page title
     */
    void setTitle(const QString& title);
    
    /**
     * @brief Set visibility
     * @param visible Visibility state
     */
    void setVisible(bool visible);
    
    /**
     * @brief Set content box
     * @param box Content box in page coordinates
     */
    void setContentBox(const QRectF& box);

private:
    class Private;
    std::unique_ptr<Private> d;
    Document* m_document;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_PAGE_H