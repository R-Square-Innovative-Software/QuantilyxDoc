/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_PDFDOCUMENT_H
#define QUANTILYX_PDFDOCUMENT_H

#include "../../core/Document.h"
#include <poppler-qt5.h>
#include <memory>

namespace QuantilyxDoc {

class PdfPage;
class PdfRenderer;
class PdfForm;
class PdfAnnotation;
class PdfBookmark;

/**
 * @brief PDF document implementation
 * 
 * Concrete implementation of Document interface for PDF files using Poppler.
 */
class PdfDocument : public Document
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit PdfDocument(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~PdfDocument() override;

    // Document interface implementation
    bool load(const QString& filePath, const QString& password = QString()) override;
    bool save(const QString& filePath = QString()) override;
    DocumentType type() const override;
    int pageCount() const override;
    Page* page(int index) const override;
    bool isLocked() const override;
    bool isEncrypted() const override;
    QString formatVersion() const override;
    bool supportsFeature(const QString& feature) const override;

    // PDF-specific functionality
    /**
     * @brief Get PDF version string
     * @return PDF version (e.g., "1.4", "1.7")
     */
    QString pdfVersion() const;

    /**
     * @brief Check if PDF is linearized (web optimized)
     * @return true if linearized
     */
    bool isLinearized() const;

    /**
     * @brief Get page layout mode
     * @return Page layout mode
     */
    Poppler::Document::PageLayout pageLayout() const;

    /**
     * @brief Get page mode
     * @return Page mode
     */
    Poppler::Document::PageMode pageMode() const;

    /**
     * @brief Get PDF producer
     * @return Producer application name
     */
    QString producer() const;

    /**
     * @brief Check if document has forms
     * @return true if contains forms
     */
    bool hasForms() const;

    /**
     * @brief Check if document has annotations
     * @return true if contains annotations
     */
    bool hasAnnotations() const;

    /**
     * @brief Check if document has embedded files
     * @return true if contains embedded files
     */
    bool hasEmbeddedFiles() const;

    /**
     * @brief Get PDF metadata as XML
     * @return XMP metadata
     */
    QString xmpMetadata() const;

    /**
     * @brief Get underlying Poppler document
     * @return Poppler document or nullptr
     */
    Poppler::Document* popplerDocument() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_PDFDOCUMENT_H