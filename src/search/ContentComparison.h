/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_CONTENTCOMPARISON_H
#define QUANTILYX_CONTENTCOMPARISON_H

#include <QObject>
#include <QList>
#include <QPair>
#include <QRectF>
#include <QFuture>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

class Document; // Forward declaration

/**
 * @brief Structure holding information about a difference found during comparison.
 */
struct Difference {
    enum Type { Text, Image, Formatting, Metadata, Structure, Other }; // Types of differences

    Type type;                    // Type of difference
    int leftPageIndex;            // Page index in the left document where difference occurs (-1 if not page-specific)
    int rightPageIndex;           // Page index in the right document where difference occurs (-1 if not page-specific)
    QRectF leftBounds;            // Bounding box in the left document where difference occurs
    QRectF rightBounds;           // Bounding box in the right document where difference occurs
    QString leftText;             // Text snippet from the left document
    QString rightText;            // Text snippet from the right document
    QString description;          // Human-readable description of the difference
    float similarityScore;        // Score indicating how similar the compared elements are (0.0 = completely different, 1.0 = identical)
};

/**
 * @brief Compares content between two documents or files.
 * 
 * Provides methods to analyze differences in text, images, formatting, structure, etc.
 * Can compare entire documents or specific regions/pages.
 */
class ContentComparison : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit ContentComparison(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ContentComparison() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global ContentComparison instance.
     */
    static ContentComparison& instance();

    /**
     * @brief Compare two documents.
     * @param leftDoc The first document to compare.
     * @param rightDoc The second document to compare.
     * @param compareText Whether to compare text content.
     * @param compareImages Whether to compare images (if applicable).
     * @param compareFormatting Whether to compare formatting (if applicable).
     * @param compareMetadata Whether to compare metadata (title, author, etc.).
     * @param compareStructure Whether to compare document structure (TOC, page count, etc.).
     * @return List of differences found.
     */
    QList<Difference> compareDocuments(Document* leftDoc, Document* rightDoc,
                                      bool compareText = true,
                                      bool compareImages = false,
                                      bool compareFormatting = false,
                                      bool compareMetadata = false,
                                      bool compareStructure = false) const;

    /**
     * @brief Compare two documents asynchronously.
     * @param leftDoc The first document to compare.
     * @param rightDoc The second document to compare.
     * @param compareText Whether to compare text content.
     * @param compareImages Whether to compare images.
     * @param compareFormatting Whether to compare formatting.
     * @param compareMetadata Whether to compare metadata.
     * @param compareStructure Whether to compare structure.
     * @return A QFuture that will hold the list of differences upon completion.
     */
    QFuture<QList<Difference>> compareDocumentsAsync(Document* leftDoc, Document* rightDoc,
                                                     bool compareText = true,
                                                     bool compareImages = false,
                                                     bool compareFormatting = false,
                                                     bool compareMetadata = false,
                                                     bool compareStructure = false) const;

    /**
     * @brief Compare two specific pages from different documents.
     * @param leftPage The first page to compare.
     * @param rightPage The second page to compare.
     * @param regionLeft Optional region to compare on the left page.
     * @param regionRight Optional region to compare on the right page.
     * @return List of differences found on the specified pages/regions.
     */
    QList<Difference> comparePages(Document* leftDoc, int leftPageIndex,
                                   Document* rightDoc, int rightPageIndex,
                                   const QRectF& regionLeft = QRectF(),
                                   const QRectF& regionRight = QRectF()) const;

    /**
     * @brief Compare specific regions within the same document.
     * @param doc The document containing the regions.
     * @param regionLeft The first region to compare.
     * @param regionRight The second region to compare.
     * @return List of differences found between the regions.
     */
    QList<Difference> compareRegionsWithinDocument(Document* doc,
                                                   const QRectF& regionLeft,
                                                   const QRectF& regionRight) const;

    /**
     * @brief Generate a visual report of the differences.
     * This could be an image highlighting differences, an HTML report, or a side-by-side comparison view.
     * @param differences The list of differences to visualize.
     * @param outputPath Path to save the report.
     * @param format Output format (e.g., "html", "png").
     * @return True if report generation was successful.
     */
    bool generateReport(const QList<Difference>& differences, const QString& outputPath, const QString& format = "html") const;

    /**
     * @brief Get the similarity threshold for determining if elements are different.
     * @return Threshold value (0.0 - 1.0).
     */
    float similarityThreshold() const;

    /**
     * @brief Set the similarity threshold for determining if elements are different.
     * Values below this threshold are considered different.
     * @param threshold Threshold value (0.0 - 1.0).
     */
    void setSimilarityThreshold(float threshold);

    /**
     * @brief Get the list of supported output formats for reports.
     * @return List of format strings (e.g., "html", "png", "json").
     */
    QStringList supportedReportFormats() const;

signals:
    /**
     * @brief Emitted when a comparison task starts.
     */
    void comparisonStarted();

    /**
     * @brief Emitted when a comparison task finishes.
     * @param differences The list of differences found.
     */
    void comparisonFinished(const QList<QuantilyxDoc::Difference>& differences);

    /**
     * @brief Emitted when a comparison task fails.
     * @param error Error message.
     */
    void comparisonFailed(const QString& error);

    /**
     * @brief Emitted periodically during a long-running comparison task.
     * @param progress Progress percentage (0-100).
     */
    void comparisonProgress(int progress);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_CONTENTCOMPARISON_H