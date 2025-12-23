/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "ContentComparison.h"
#include "../core/Document.h"
#include "../core/Page.h"
#include "../core/Logger.h"
#include <QImage>
#include <QPainter>
#include <QTextDocument> // For basic text comparison
#include <QTextBlock>
#include <QTextFragment>
#include <QRegularExpression>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

namespace QuantilyxDoc {

class ContentComparison::Private {
public:
    Private(ContentComparison* q_ptr)
        : q(q_ptr), similarityThresholdVal(0.8f) {}

    ContentComparison* q;
    mutable QMutex mutex; // Protect access if needed during comparison
    float similarityThresholdVal;

    // Helper to calculate similarity between two strings (e.g., using Levenshtein distance)
    float calculateStringSimilarity(const QString& str1, const QString& str2) const {
        if (str1 == str2) return 1.0f;
        if (str1.isEmpty() && str2.isEmpty()) return 1.0f;
        if (str1.isEmpty() || str2.isEmpty()) return 0.0f;

        // Use Qt's built-in fuzzy matching or implement Levenshtein distance
        // Qt doesn't have a direct Levenshtein function, so we'll use a simple ratio of common characters.
        // A more robust implementation would use an algorithm like difflib or a dedicated library.
        int len1 = str1.size();
        int len2 = str2.size();
        int commonLength = 0;
        int minLength = qMin(len1, len2);

        for (int i = 0; i < minLength; ++i) {
            if (str1[i] == str2[i]) {
                commonLength++;
            }
        }

        float similarity = static_cast<float>(commonLength) / qMax(len1, len2);
        LOG_DEBUG("ContentComparison: String similarity for '" << str1.left(20) << "' vs '" << str2.left(20) << "' = " << similarity);
        return similarity;
    }

    // Helper to compare two images (e.g., using SSIM or simple pixel difference)
    float calculateImageSimilarity(const QImage& img1, const QImage& img2) const {
        if (img1.isNull() || img2.isNull()) return img1.isNull() == img2.isNull() ? 1.0f : 0.0f;
        if (img1.size() != img2.size()) return 0.0f; // Different sizes are completely different

        // For simplicity, compare raw pixel data. This is very basic.
        // A real implementation would use Structural Similarity Index Measure (SSIM) or other perceptual metrics.
        if (img1 == img2) return 1.0f;

        QImage converted1 = img1.convertToFormat(QImage::Format_ARGB32);
        QImage converted2 = img2.convertToFormat(QImage::Format_ARGB32);

        const uchar* data1 = converted1.constBits();
        const uchar* data2 = converted2.constBits();
        int byteCount = converted1.sizeInBytes();

        int differentBytes = 0;
        for (int i = 0; i < byteCount; ++i) {
            if (data1[i] != data2[i]) {
                differentBytes++;
            }
        }

        float similarity = 1.0f - (static_cast<float>(differentBytes) / byteCount);
        LOG_DEBUG("ContentComparison: Image similarity = " << similarity);
        return similarity;
    }

    // Helper to compare two pieces of text
    QList<Difference> compareText(const QString& leftText, const QString& rightText, int leftPage, int rightPage) const {
        QList<Difference> diffs;
        if (leftText == rightText) {
            LOG_DEBUG("ContentComparison: Text on pages " << leftPage << " and " << rightPage << " are identical.");
            return diffs; // No differences
        }

        // Use QTextDocument for a basic comparison (or a diff library for more sophistication)
        QTextDocument doc1, doc2;
        doc1.setPlainText(leftText);
        doc2.setPlainText(rightText);

        // A very simple line-by-line comparison for demonstration
        QStringList lines1 = leftText.split('\n');
        QStringList lines2 = rightText.split('\n');

        int maxLines = qMax(lines1.size(), lines2.size());
        for (int i = 0; i < maxLines; ++i) {
            QString line1 = (i < lines1.size()) ? lines1[i] : QString();
            QString line2 = (i < lines2.size()) ? lines2[i] : QString();

            if (line1 != line2) {
                float simScore = calculateStringSimilarity(line1, line2);
                if (simScore < similarityThresholdVal) {
                    Difference diff;
                    diff.type = Difference::Text;
                    diff.leftPageIndex = leftPage;
                    diff.rightPageIndex = rightPage;
                    diff.leftText = line1;
                    diff.rightText = line2;
                    diff.description = QString("Text difference at line %1: '%2' vs '%3'").arg(i).arg(line1).arg(line2);
                    diff.similarityScore = simScore;
                    diffs.append(diff);
                    LOG_DEBUG("ContentComparison: Found text diff: " << diff.description);
                }
            }
        }
        return diffs;
    }

    // Helper to compare two pages (render them and compare visually/textually)
    QList<Difference> comparePagesInternal(Document* leftDoc, int leftPageIndex, Document* rightDoc, int rightPageIndex) const {
        QList<Difference> allDiffs;

        Page* leftPage = leftDoc->page(leftPageIndex);
        Page* rightPage = rightDoc->page(rightPageIndex);

        if (!leftPage || !rightPage) {
            LOG_WARN("ContentComparison: One of the pages to compare is null.");
            return allDiffs;
        }

        // Compare text content
        QString leftText = leftPage->text();
        QString rightText = rightPage->text();
        allDiffs.append(compareText(leftText, rightText, leftPageIndex, rightPageIndex));

        // Compare rendered images (if needed)
        // QImage leftImage = leftPage->render(width, height, dpi); // Need consistent render settings
        // QImage rightImage = rightPage->render(width, height, dpi);
        // float imageSim = calculateImageSimilarity(leftImage, rightImage);
        // if (imageSim < similarityThresholdVal) {
        //     Difference imgDiff;
        //     imgDiff.type = Difference::Image;
        //     imgDiff.leftPageIndex = leftPageIndex;
        //     imgDiff.rightPageIndex = rightPageIndex;
        //     imgDiff.description = QString("Image difference on pages %1 and %2, similarity: %3").arg(leftPageIndex).arg(rightPageIndex).arg(imageSim);
        //     imgDiff.similarityScore = imageSim;
        //     allDiffs.append(imgDiff);
        // }

        // Compare other properties like size, rotation, annotations (if applicable and accessible via Page interface)
        // QSizeF leftSize = leftPage->size();
        // QSizeF rightSize = rightPage->size();
        // if (leftSize != rightSize) {
        //     Difference sizeDiff;
        //     sizeDiff.type = Difference::Structure;
        //     sizeDiff.leftPageIndex = leftPageIndex;
        //     sizeDiff.rightPageIndex = rightPageIndex;
        //     sizeDiff.description = QString("Page size difference: %1 vs %2").arg(leftSize).arg(rightSize);
        //     sizeDiff.similarityScore = 0.0f; // Completely different
        //     allDiffs.append(sizeDiff);
        // }

        return allDiffs;
    }
};

// Static instance pointer
ContentComparison* ContentComparison::s_instance = nullptr;

ContentComparison& ContentComparison::instance()
{
    if (!s_instance) {
        s_instance = new ContentComparison();
    }
    return *s_instance;
}

ContentComparison::ContentComparison(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    LOG_INFO("ContentComparison created.");
}

ContentComparison::~ContentComparison()
{
    LOG_INFO("ContentComparison destroyed.");
}

QList<Difference> ContentComparison::compareDocuments(Document* leftDoc, Document* rightDoc,
                                                     bool compareText,
                                                     bool compareImages,
                                                     bool compareFormatting,
                                                     bool compareMetadata,
                                                     bool compareStructure) const
{
    if (!leftDoc || !rightDoc) {
        LOG_ERROR("ContentComparison::compareDocuments: Null document provided.");
        emit comparisonFailed("Null document provided.");
        return {}; // Return empty list
    }

    emit comparisonStarted();

    QList<Difference> allDifferences;

    if (compareText || compareImages) { // Compare page by page
        int pageCount1 = leftDoc->pageCount();
        int pageCount2 = rightDoc->pageCount();
        int maxPages = qMax(pageCount1, pageCount2);

        for (int i = 0; i < maxPages; ++i) {
            bool pageExists1 = (i < pageCount1);
            bool pageExists2 = (i < pageCount2);

            if (!pageExists1 || !pageExists2) {
                Difference pageCountDiff;
                pageCountDiff.type = Difference::Structure;
                pageCountDiff.leftPageIndex = pageExists1 ? i : -1;
                pageCountDiff.rightPageIndex = pageExists2 ? i : -1;
                pageCountDiff.description = QString("Page count mismatch: Document 1 has %1 pages, Document 2 has %2 pages.").arg(pageCount1).arg(pageCount2);
                pageCountDiff.similarityScore = 0.0f;
                allDifferences.append(pageCountDiff);
                continue;
            }

            // Compare the specific page
            QList<Difference> pageDiffs = d->comparePagesInternal(leftDoc, i, rightDoc, i);
            allDifferences.append(pageDiffs);
        }
    }

    // Compare metadata if requested
    if (compareMetadata) {
        if (leftDoc->title() != rightDoc->title()) {
            Difference titleDiff;
            titleDiff.type = Difference::Metadata;
            titleDiff.description = QString("Title differs: '%1' vs '%2'").arg(leftDoc->title()).arg(rightDoc->title());
            titleDiff.similarityScore = d->calculateStringSimilarity(leftDoc->title(), rightDoc->title());
            allDifferences.append(titleDiff);
        }
        // Compare other metadata fields similarly...
    }

    // Compare overall structure if requested
    if (compareStructure) {
        // Compare TOC, page count (already done above), etc.
        if (leftDoc->hasTableOfContents() != rightDoc->hasTableOfContents()) {
            Difference tocDiff;
            tocDiff.type = Difference::Structure;
            tocDiff.description = QString("Table of Contents presence differs: Left=%1, Right=%2").arg(leftDoc->hasTableOfContents()).arg(rightDoc->hasTableOfContents());
            tocDiff.similarityScore = 0.0f;
            allDifferences.append(tocDiff);
        }
        // Compare TOC content if both exist...
    }

    emit comparisonFinished(allDifferences);
    LOG_INFO("ContentComparison: Compared documents '" << leftDoc->title() << "' and '" << rightDoc->title() << "', found " << allDifferences.size() << " differences.");
    return allDifferences;
}

QFuture<QList<Difference>> ContentComparison::compareDocumentsAsync(Document* leftDoc, Document* rightDoc,
                                                                   bool compareText,
                                                                   bool compareImages,
                                                                   bool compareFormatting,
                                                                   bool compareMetadata,
                                                                   bool compareStructure) const
{
    return QtConcurrent::run([this, leftDoc, rightDoc, compareText, compareImages, compareFormatting, compareMetadata, compareStructure]() {
        return this->compareDocuments(leftDoc, rightDoc, compareText, compareImages, compareFormatting, compareMetadata, compareStructure);
    });
}

QList<Difference> ContentComparison::comparePages(Document* leftDoc, int leftPageIndex,
                                                  Document* rightDoc, int rightPageIndex,
                                                  const QRectF& regionLeft,
                                                  const QRectF& regionRight) const
{
    // This is similar to the internal page comparison but allows specifying regions.
    // For now, let's call the internal function which compares full pages.
    // Region-specific comparison would require rendering specific areas and comparing those images/text snippets.
    Q_UNUSED(regionLeft);
    Q_UNUSED(regionRight);
    return d->comparePagesInternal(leftDoc, leftPageIndex, rightDoc, rightPageIndex);
}

QList<Difference> ContentComparison::compareRegionsWithinDocument(Document* doc,
                                                                 const QRectF& regionLeft,
                                                                 const QRectF& regionRight) const
{
    // Compare two regions within the same document page or across pages.
    // This requires rendering the regions and comparing the resulting images/text.
    Q_UNUSED(doc);
    Q_UNUSED(regionLeft);
    Q_UNUSED(regionRight);
    LOG_WARN("ContentComparison::compareRegionsWithinDocument: Not implemented.");
    return {}; // Placeholder
}

bool ContentComparison::generateReport(const QList<Difference>& differences, const QString& outputPath, const QString& format) const
{
    // Generate an HTML or image report showing the differences.
    // This is complex and depends on the output format.
    // For HTML, it might involve creating a web page with side-by-side views and highlighted differences.
    // For images, it might involve creating composite images showing both documents with differences marked.
    Q_UNUSED(differences);
    Q_UNUSED(outputPath);
    Q_UNUSED(format);
    LOG_WARN("ContentComparison::generateReport: Not implemented.");
    return false; // Placeholder
}

float ContentComparison::similarityThreshold() const
{
    QMutexLocker locker(&d->mutex);
    return d->similarityThresholdVal;
}

void ContentComparison::setSimilarityThreshold(float threshold)
{
    QMutexLocker locker(&d->mutex);
    if (d->similarityThresholdVal != threshold) {
        d->similarityThresholdVal = threshold;
        LOG_INFO("ContentComparison: Similarity threshold set to " << threshold);
    }
}

QStringList ContentComparison::supportedReportFormats() const
{
    return QStringList() << "html" << "json"; // Add more as implemented
}

} // namespace QuantilyxDoc