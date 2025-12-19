/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef QUANTILYX_DOCUMENT_H
#define QUANTILYX_DOCUMENT_H

#include <QObject>
#include <QString>
#include <QSize>
#include <QImage>
#include <QList>
#include <memory>

namespace QuantilyxDoc {

// Forward declarations
class Page;
class Annotation;

/**
 * @brief Document type enumeration
 */
enum class DocumentType
{
    Unknown,
    PDF,
    EPUB,
    DjVu,
    CBZ,
    CBR,
    PostScript,
    XPS,
    CHM,
    Markdown,
    FictionBook,
    Mobi,
    Image,
    CAD,
    Office
};

/**
 * @brief Base document class
 * 
 * Abstract base class for all document types. Provides common interface
 * for document operations like opening, closing, rendering, and editing.
 */
class Document : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Document state enumeration
     */
    enum State
    {
        Unloaded,
        Loading,
        Loaded,
        Error
    };

    /**
     * @brief Destructor
     */
    virtual ~Document();

    /**
     * @brief Load document from file
     * @param filePath Path to document file
     * @param password Password for encrypted documents
     * @return true if loaded successfully
     */
    virtual bool load(const QString& filePath, const QString& password = QString()) = 0;

    /**
     * @brief Save document to file
     * @param filePath Path to save to (empty = save to current path)
     * @return true if saved successfully
     */
    virtual bool save(const QString& filePath = QString()) = 0;

    /**
     * @brief Close document
     */
    virtual void close();

    /**
     * @brief Get document type
     * @return Document type
     */
    virtual DocumentType type() const = 0;

    /**
     * @brief Get document file path
     * @return File path
     */
    QString filePath() const;

    /**
     * @brief Get document title
     * @return Title
     */
    virtual QString title() const;

    /**
     * @brief Get document author
     * @return Author
     */
    virtual QString author() const;

    /**
     * @brief Get document subject
     * @return Subject
     */
    virtual QString subject() const;

    /**
     * @brief Get document keywords
     * @return Keywords
     */
    virtual QStringList keywords() const;

    /**
     * @brief Get creation date
     * @return Creation date
     */
    virtual QDateTime creationDate() const;

    /**
     * @brief Get modification date
     * @return Modification date
     */
    virtual QDateTime modificationDate() const;

    /**
     * @brief Get number of pages
     * @return Page count
     */
    virtual int pageCount() const = 0;

    /**
     * @brief Get page by index
     * @param index Page index (0-based)
     * @return Page pointer or nullptr
     */
    virtual Page* page(int index) const = 0;

    /**
     * @brief Get current page index
     * @return Current page index
     */
    int currentPageIndex() const;

    /**
     * @brief Set current page index
     * @param index Page index
     */
    void setCurrentPageIndex(int index);

    /**
     * @brief Check if document is modified
     * @return true if modified
     */
    bool isModified() const;

    /**
     * @brief Set modified state
     * @param modified Modified state
     */
    void setModified(bool modified);

    /**
     * @brief Check if document is locked (read-only)
     * @return true if locked
     */
    virtual bool isLocked() const;

    /**
     * @brief Check if document is encrypted
     * @return true if encrypted
     */
    virtual bool isEncrypted() const;

    /**
     * @brief Get document state
     * @return Current state
     */
    State state() const;

    /**
     * @brief Get last error message
     * @return Error message
     */
    QString lastError() const;

    /**
     * @brief Get file size in bytes
     * @return File size
     */
    qint64 fileSize() const;

    /**
     * @brief Get document format version
     * @return Format version string
     */
    virtual QString formatVersion() const;

    /**
     * @brief Check if document supports feature
     * @param feature Feature name
     * @return true if supported
     */
    virtual bool supportsFeature(const QString& feature) const;

    // Annotation support
    /**
     * @brief Get all annotations
     * @return List of annotations
     */
    virtual QList<Annotation*> annotations() const;

    /**
     * @brief Add annotation
     * @param annotation Annotation to add
     */
    virtual void addAnnotation(Annotation* annotation);

    /**
     * @brief Remove annotation
     * @param annotation Annotation to remove
     */
    virtual void removeAnnotation(Annotation* annotation);

    // Table of contents support
    /**
     * @brief Check if document has table of contents
     * @return true if has TOC
     */
    virtual bool hasTableOfContents() const;

    /**
     * @brief Get table of contents
     * @return TOC structure
     */
    virtual QVariantList tableOfContents() const;

    // Bookmarks support
    /**
     * @brief Get bookmarks
     * @return List of bookmarks
     */
    virtual QStringList bookmarks() const;

    /**
     * @brief Add bookmark
     * @param name Bookmark name
     * @param pageIndex Page index
     */
    virtual void addBookmark(const QString& name, int pageIndex);

    /**
     * @brief Remove bookmark
     * @param name Bookmark name
     */
    virtual void removeBookmark(const QString& name);

    // Search support
    /**
     * @brief Search for text in document
     * @param text Text to search
     * @param caseSensitive Case sensitive search
     * @param wholeWords Whole words only
     * @return List of page indices where text was found
     */
    virtual QList<int> search(const QString& text, 
                             bool caseSensitive = false,
                             bool wholeWords = false) const;

signals:
    /**
     * @brief Emitted when document is loaded
     */
    void loaded();

    /**
     * @brief Emitted when document loading fails
     * @param error Error message
     */
    void loadFailed(const QString& error);

    /**
     * @brief Emitted when document is saved
     */
    void saved();

    /**
     * @brief Emitted when document is modified
     */
    void modified();

    /**
     * @brief Emitted when current page changes
     * @param index New page index
     */
    void currentPageChanged(int index);

    /**
     * @brief Emitted when document is closed
     */
    void closed();

protected:
    /**
     * @brief Protected constructor
     * @param parent Parent object
     */
    explicit Document(QObject* parent = nullptr);

    /**
     * @brief Set document state
     * @param state New state
     */
    void setState(State state);

    /**
     * @brief Set last error
     * @param error Error message
     */
    void setLastError(const QString& error);

    /**
     * @brief Set file path
     * @param path File path
     */
    void setFilePath(const QString& path);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_DOCUMENT_H