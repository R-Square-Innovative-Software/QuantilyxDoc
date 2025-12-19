/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_DOCUMENTVIEW_H
#define QUANTILYX_DOCUMENTVIEW_H

#include <QWidget>
#include <memory>

namespace QuantilyxDoc {

class Document;
class Page;
class DocumentRenderer;

/**
 * @brief Document view widget
 * 
 * Handles rendering and interaction with documents. Supports multiple viewing modes,
 * zoom levels, and navigation.
 */
class DocumentView : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief View modes
     */
    enum ViewMode
    {
        SinglePage,         ///< Show one page at a time
        FacingPages,        ///< Show two pages side by side (like a book)
        Continuous,         ///< Continuous scroll through all pages
        Presentation        ///< Full-screen presentation mode
    };
    
    /**
     * @brief Zoom modes
     */
    enum ZoomMode
    {
        FitPage,            ///< Fit entire page to view
        FitWidth,           ///< Fit page width to view
        FitVisible,         ///< Fit visible content to view
        CustomZoom          ///< Custom zoom percentage
    };

    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit DocumentView(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DocumentView();

    /**
     * @brief Set document to view
     * @param document Document to display
     */
    void setDocument(Document* document);

    /**
     * @brief Get current document
     * @return Current document or nullptr
     */
    Document* document() const;

    /**
     * @brief Set view mode
     * @param mode View mode
     */
    void setViewMode(ViewMode mode);

    /**
     * @brief Get current view mode
     * @return Current view mode
     */
    ViewMode viewMode() const;

    /**
     * @brief Set zoom mode
     * @param mode Zoom mode
     */
    void setZoomMode(ZoomMode mode);

    /**
     * @brief Set custom zoom level
     * @param zoom Zoom percentage (e.g., 100 for 100%)
     */
    void setZoomLevel(qreal zoom);

    /**
     * @brief Get current zoom level
     * @return Current zoom percentage
     */
    qreal zoomLevel() const;

    /**
     * @brief Go to specific page
     * @param pageIndex Page index (0-based)
     */
    void goToPage(int pageIndex);

    /**
     * @brief Get current page index
     * @return Current page index
     */
    int currentPageIndex() const;

    /**
     * @brief Get total page count
     * @return Total number of pages
     */
    int pageCount() const;

    /**
     * @brief Rotate view
     * @param degrees Rotation in degrees (90, 180, 270)
     */
    void rotateView(int degrees);

    /**
     * @brief Set page spacing
     * @param spacing Spacing between pages in pixels
     */
    void setPageSpacing(int spacing);

signals:
    /**
     * @brief Emitted when current page changes
     * @param pageIndex New page index
     */
    void currentPageChanged(int pageIndex);

    /**
     * @brief Emitted when zoom level changes
     * @param zoomLevel New zoom level
     */
    void zoomLevelChanged(qreal zoomLevel);

    /**
     * @brief Emitted when document is modified
     */
    void documentModified();

protected:
    /**
     * @brief Handle paint event
     * @param event Paint event
     */
    void paintEvent(QPaintEvent* event) override;

    /**
     * @brief Handle resize event
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent* event) override;

    /**
     * @brief Handle mouse wheel event
     * @param event Wheel event
     */
    void wheelEvent(QWheelEvent* event) override;

    /**
     * @brief Handle key press event
     * @param event Key press event
     */
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    /**
     * @brief Handle document loading
     */
    void onDocumentLoaded();

    /**
     * @brief Handle document modification
     */
    void onDocumentModified();

    /**
     * @brief Update viewport
     */
    void updateViewport();

    /**
     * @brief Render visible pages
     */
    void renderVisiblePages();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_DOCUMENTVIEW_H