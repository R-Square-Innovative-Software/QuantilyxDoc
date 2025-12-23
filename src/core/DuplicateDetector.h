/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_DUPLICATEDETECTOR_H
#define QUANTILYX_DUPLICATEDETECTOR_H

#include <QObject>
#include <QList>
#include <QPair>
#include <QHash>
#include <QMutex>
#include <memory>

namespace QuantilyxDoc {

class Document; // Forward declaration

/**
 * @brief Represents a group of duplicate/similar documents found by the detector.
 */
struct DuplicateGroup {
    QList<Document*> documents; // List of documents in this group
    float similarityScore;      // Overall similarity score for the group (0.0 - 1.0, 1.0 = exact duplicate)
    QString representativeFilePath; // A path chosen as the "representative" of the group
};

/**
 * @brief Finds duplicate or similar documents based on content, metadata, or file properties.
 * 
 * Uses techniques like file hashing, content fingerprinting, or metadata comparison
 * to identify documents that are identical or highly similar.
 */
class DuplicateDetector : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit DuplicateDetector(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~DuplicateDetector() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global DuplicateDetector instance.
     */
    static DuplicateDetector& instance();

    /**
     * @brief Analyze a single document for duplicates against the database/index.
     * @param document The document to analyze.
     * @param similarityThreshold Minimum similarity score to consider a match (0.0 - 1.0).
     * @return List of documents considered duplicates/similars to the input document.
     */
    QList<Document*> findDuplicatesForDocument(Document* document, float similarityThreshold = 0.95f) const;

    /**
     * @brief Analyze a list of documents to find all duplicates/similars among them.
     * @param documents The list of documents to analyze.
     * @param similarityThreshold Minimum similarity score to consider a match (0.0 - 1.0).
     * @return List of groups of duplicate/similar documents.
     */
    QList<DuplicateGroup> findDuplicatesInList(const QList<Document*>& documents, float similarityThreshold = 0.95f) const;

    /**
     * @brief Analyze documents in a specific directory for duplicates.
     * @param directoryPath Path to the directory to scan.
     * @param recursive Whether to scan subdirectories.
     * @param similarityThreshold Minimum similarity score to consider a match (0.0 - 1.0).
     * @return List of groups of duplicate/similar documents.
     */
    QList<DuplicateGroup> findDuplicatesInDirectory(const QString& directoryPath, bool recursive = true, float similarityThreshold = 0.95f) const;

    /**
     * @brief Check if the detector is currently analyzing documents.
     * @return True if analysis is in progress.
     */
    bool isAnalyzing() const;

    /**
     * @brief Get the number of documents analyzed in the last run.
     * @return Count of analyzed documents.
     */
    int lastAnalysisDocumentCount() const;

    /**
     * @brief Get the number of duplicate groups found in the last run.
     * @return Count of duplicate groups.
     */
    int lastAnalysisDuplicateCount() const;

    /**
     * @brief Get the similarity threshold used for detection.
     * @return Threshold value (0.0 - 1.0).
     */
    float similarityThreshold() const;

    /**
     * @brief Set the similarity threshold used for detection.
     * @param threshold Threshold value (0.0 - 1.0).
     */
    void setSimilarityThreshold(float threshold);

    /**
     * @brief Get the list of supported analysis methods (e.g., "hash", "text", "metadata").
     * @return List of method names.
     */
    QStringList supportedMethods() const;

    /**
     * @brief Get the currently active analysis method.
     * @return Method name.
     */
    QString activeMethod() const;

    /**
     * @brief Set the active analysis method.
     * @param method Method name (e.g., "hash", "text").
     */
    void setActiveMethod(const QString& method);

signals:
    /**
     * @brief Emitted when a document analysis starts.
     * @param document The document being analyzed.
     */
    void analysisStarted(QuantilyxDoc::Document* document);

    /**
     * @brief Emitted when a document analysis finishes.
     * @param document The document that was analyzed.
     * @param duplicates List of duplicate/similar documents found.
     */
    void analysisFinished(QuantilyxDoc::Document* document, const QList<QuantilyxDoc::Document*>& duplicates);

    /**
     * @brief Emitted when a batch analysis (directory/list) starts.
     */
    void batchAnalysisStarted();

    /**
     * @brief Emitted when a batch analysis finishes.
     * @param duplicateGroups List of found duplicate groups.
     */
    void batchAnalysisFinished(const QList<QuantilyxDoc::DuplicateGroup>& duplicateGroups);

    /**
     * @brief Emitted periodically during a long-running analysis task.
     * @param progress Progress percentage (0-100).
     */
    void analysisProgress(int progress);

    /**
     * @brief Emitted when a potential duplicate is found during analysis.
     * @param document1 First document in the pair.
     * @param document2 Second document in the pair.
     * @param similarityScore The calculated similarity score.
     */
    void duplicateFound(QuantilyxDoc::Document* document1, QuantilyxDoc::Document* document2, float similarityScore);

private:
    class Private;
    std::unique_ptr<Private> d;

    // Helper to calculate a hash for a document (e.g., SHA256 of file content)
    QString calculateFileHash(const QString& filePath) const;

    // Helper to extract and fingerprint content (e.g., text, images) for similarity comparison
    QString fingerprintContent(Document* document) const;

    // Helper to compare two fingerprints and return a similarity score
    float compareFingerprints(const QString& fp1, const QString& fp2) const;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_DUPLICATEDETECTOR_H