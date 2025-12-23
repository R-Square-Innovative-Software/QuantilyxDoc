/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "DuplicateDetector.h"
#include "Document.h"
#include "Logger.h"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextDocument> // For text fingerprinting
#include <QTextBlock>
#include <QTextFragment>
#include <QMutex>
#include <QMutexLocker>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

namespace QuantilyxDoc {

class DuplicateDetector::Private {
public:
    Private(DuplicateDetector* q_ptr)
        : q(q_ptr), analyzing(false), lastDocCount(0), lastDupCount(0), similarityThresholdVal(0.95f), activeMethodStr("hash") {}

    DuplicateDetector* q;
    mutable QMutex mutex; // Protect access to state and results during analysis
    bool analyzing;
    int lastDocCount;
    int lastDupCount;
    float similarityThresholdVal;
    QString activeMethodStr;
    QList<DuplicateGroup> lastResults;

    // Helper to calculate SHA256 hash of a file
    QString calculateFileHash(const QString& filePath) const {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            LOG_ERROR("DuplicateDetector: Failed to open file for hashing: " << filePath);
            return QString();
        }

        QCryptographicHash hasher(QCryptographicHash::Sha256);
        if (!hasher.addData(&file)) {
            LOG_ERROR("DuplicateDetector: Failed to calculate hash for: " << filePath);
            return QString();
        }

        QString hash = hasher.result().toHex();
        LOG_DEBUG("DuplicateDetector: Hashed " << filePath << " -> " << hash);
        return hash;
    }

    // Helper to extract text content for fingerprinting (simple approach)
    QString fingerprintTextContent(Document* document) const {
        // This is a very basic fingerprint, just concatenating all text.
        // A more sophisticated approach might involve n-grams, TF-IDF, or LSH.
        QString fullText;
        for (int i = 0; i < document->pageCount(); ++i) {
            Page* page = document->page(i);
            if (page) {
                fullText += page->text(); // Hypothetical Page::text() method
            }
        }
        // Normalize: remove extra whitespace, convert to lower case, etc.
        fullText = fullText.simplified().toLower();
        // Calculate a hash of the normalized text as a "fingerprint"
        QCryptographicHash hasher(QCryptographicHash::Sha256);
        hasher.addData(fullText.toUtf8());
        QString fingerprint = hasher.result().toHex();
        LOG_DEBUG("DuplicateDetector: Fingerprinted text for " << document->filePath() << " -> " << fingerprint.left(16) << "...");
        return fingerprint;
    }

    // Helper to compare two fingerprints (hashes)
    // For hash-based comparison, they are either identical (1.0) or different (0.0).
    // For more complex fingerprints (e.g., text similarity), this would calculate a score.
    float compareHashes(const QString& hash1, const QString& hash2) const {
        return (hash1 == hash2) ? 1.0f : 0.0f;
    }

    // Helper to compare two text fingerprints (simplified example using Hamming distance on hashes)
    // This is still a very basic comparison. Real similarity requires more advanced algorithms.
    float compareTextFingerprints(const QString& fp1, const QString& fp2) const {
        if (fp1 == fp2) return 1.0f;
        // A simple similarity based on number of matching characters in the hash
        // This is not a good metric for actual text similarity.
        // int matchingChars = 0;
        // int minLength = qMin(fp1.length(), fp2.length());
        // for (int i = 0; i < minLength; ++i) {
        //     if (fp1[i] == fp2[i]) matchingChars++;
        // }
        // float score = static_cast<float>(matchingChars) / minLength;
        // A better approach for text would be using difflib, cosine similarity on TF-IDF vectors, etc.
        // For now, return 0.0 if not exactly equal.
        LOG_DEBUG("DuplicateDetector: Comparing text fingerprints (basic): " << fp1.left(16) << " vs " << fp2.left(16));
        return 0.0f; // Placeholder, implement proper text similarity if needed
    }
};

// Static instance pointer
DuplicateDetector* DuplicateDetector::s_instance = nullptr;

DuplicateDetector& DuplicateDetector::instance()
{
    if (!s_instance) {
        s_instance = new DuplicateDetector();
    }
    return *s_instance;
}

DuplicateDetector::DuplicateDetector(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    LOG_INFO("DuplicateDetector created.");
}

DuplicateDetector::~DuplicateDetector()
{
    LOG_INFO("DuplicateDetector destroyed.");
}

QList<Document*> DuplicateDetector::findDuplicatesForDocument(Document* document, float similarityThreshold) const
{
    if (!document) {
        LOG_ERROR("DuplicateDetector::findDuplicatesForDocument: Null document provided.");
        return {};
    }

    // This would require a pre-built index/database of fingerprints from other documents.
    // For now, this is a stub that might compare against a list of known documents.
    LOG_WARN("DuplicateDetector::findDuplicatesForDocument: Requires a pre-built index/database. Stub implementation.");
    Q_UNUSED(similarityThreshold);
    return {}; // Placeholder
}

QList<DuplicateGroup> DuplicateDetector::findDuplicatesInList(const QList<Document*>& documents, float similarityThreshold) const
{
    QMutexLocker locker(&d->mutex);
    if (d->analyzing) {
        LOG_WARN("DuplicateDetector::findDuplicatesInList: Analysis already in progress.");
        return {};
    }
    d->analyzing = true;
    d->lastDocCount = documents.size();
    d->lastDupCount = 0;
    d->lastResults.clear();

    emit batchAnalysisStarted();

    // Use QtConcurrent for parallel processing of documents if possible
    // For simplicity here, we'll do a sequential, pairwise comparison.
    QList<DuplicateGroup> groups;
    QHash<QString, DuplicateGroup> hashToGroup; // Map fingerprint -> group for hash-based method

    for (int i = 0; i < documents.size(); ++i) {
        Document* doc1 = documents[i];
        if (!doc1) continue;

        emit analysisStarted(doc1);

        QString fingerprint1;
        if (d->activeMethodStr == "hash") {
            fingerprint1 = d->calculateFileHash(doc1->filePath());
        } else if (d->activeMethodStr == "text") {
            fingerprint1 = d->q->fingerprintContent(doc1);
        } else {
            LOG_WARN("DuplicateDetector::findDuplicatesInList: Unknown method: " << d->activeMethodStr);
            continue;
        }

        if (fingerprint1.isEmpty()) continue; // Could not fingerprint

        // Check against already processed documents
        bool addedToExistingGroup = false;
        if (d->activeMethodStr == "hash") {
            // For hash, if fingerprint matches, it's a duplicate
            if (hashToGroup.contains(fingerprint1)) {
                hashToGroup[fingerprint1].documents.append(doc1);
                addedToExistingGroup = true;
            } else {
                DuplicateGroup newGroup;
                newGroup.documents.append(doc1);
                newGroup.similarityScore = 1.0f; // Exact hash match
                newGroup.representativeFilePath = doc1->filePath();
                hashToGroup.insert(fingerprint1, newGroup);
                groups.append(newGroup);
                d->lastDupCount++; // A new group with more than one doc is a duplicate group
            }
        } else { // text or other method requiring similarity comparison
            for (auto& group : groups) {
                if (!group.documents.isEmpty()) {
                    Document* repDoc = group.documents.first(); // Use first doc in group as representative for comparison
                    QString repFingerprint;
                    if (d->activeMethodStr == "hash") {
                        repFingerprint = d->calculateFileHash(repDoc->filePath());
                    } else if (d->activeMethodStr == "text") {
                        repFingerprint = d->q->fingerprintContent(repDoc);
                    }

                    float similarity = (d->activeMethodStr == "hash") ?
                                      d->compareHashes(fingerprint1, repFingerprint) :
                                      d->compareTextFingerprints(fingerprint1, repFingerprint);

                    if (similarity >= similarityThreshold) {
                        group.documents.append(doc1);
                        group.similarityScore = qMax(group.similarityScore, similarity); // Update score to highest found
                        addedToExistingGroup = true;
                        d->lastDupCount++; // Increment for each new doc added to an existing group
                        break; // Found a group, stop looking
                    }
                }
            }
            if (!addedToExistingGroup) {
                // Create a new group for this document
                DuplicateGroup newGroup;
                newGroup.documents.append(doc1);
                newGroup.similarityScore = 0.0f; // Will be updated if merged with another
                newGroup.representativeFilePath = doc1->filePath();
                groups.append(newGroup);
            }
        }

        emit analysisFinished(doc1, addedToExistingGroup ? groups.last().documents : QList<Document*>());
    }

    // Finalize groups: remove groups with only one document (not duplicates)
    auto it = groups.begin();
    while (it != groups.end()) {
        if (it->documents.size() <= 1) {
            it = groups.erase(it);
        } else {
            ++it;
        }
    }
    d->lastDupCount = groups.size();

    d->lastResults = groups;
    d->analyzing = false;

    emit batchAnalysisFinished(groups);
    LOG_INFO("DuplicateDetector: Analyzed " << documents.size() << " documents, found " << groups.size() << " duplicate groups.");
    return groups;
}

QList<DuplicateGroup> DuplicateDetector::findDuplicatesInDirectory(const QString& directoryPath, bool recursive, float similarityThreshold) const
{
    QDir dir(directoryPath);
    if (!dir.exists()) {
        LOG_ERROR("DuplicateDetector::findDuplicatesInDirectory: Directory does not exist: " << directoryPath);
        return {};
    }

    QStringList filters = {"*.pdf", "*.epub", "*.djvu", "*.cbz", "*.cbr", "*.ps", "*.xps", "*.chm", "*.md", "*.fb2", "*.mobi", "*.txt", "*.rtf"}; // Add more as needed
    QStringList fileNames = dir.entryList(filters, QDir::Files | (recursive ? QDir::NoDotAndDotDot | QDir::AllDirs : QDir::NoDotAndDotDot), QDir::Name);
    // Recursively get files if requested (QDirIterator is better for this)
    if (recursive) {
        fileNames.clear(); // Clear initial list
        QDirIterator it(directoryPath, filters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            fileNames.append(it.next());
        }
    }

    // This requires loading Document objects for each file to pass to findDuplicatesInList.
    // This is expensive. A more efficient method would fingerprint files directly without full Document loading.
    LOG_WARN("DuplicateDetector::findDuplicatesInDirectory: Loading full Document objects for each file. Consider optimizing by fingerprinting files directly.");
    QList<Document*> docs;
    // QList<Document*> docs = loadDocumentsFromFilePaths(fileNames); // Hypothetical function to load docs from paths

    // For now, let's assume we have a list of Document* objects loaded from the file paths.
    // This is a critical performance bottleneck and needs a better implementation.
    // A real implementation would fingerprint files directly (e.g., hash, text fingerprint) and store/process them without Document objects.
    // This is a stub.
    Q_UNUSED(docs);
    Q_UNUSED(similarityThreshold);
    LOG_WARN("DuplicateDetector::findDuplicatesInDirectory: Requires efficient file fingerprinting. Returning empty list.");
    return {}; // Placeholder
}

bool DuplicateDetector::isAnalyzing() const
{
    QMutexLocker locker(&d->mutex);
    return d->analyzing;
}

int DuplicateDetector::lastAnalysisDocumentCount() const
{
    QMutexLocker locker(&d->mutex);
    return d->lastDocCount;
}

int DuplicateDetector::lastAnalysisDuplicateCount() const
{
    QMutexLocker locker(&d->mutex);
    return d->lastDupCount;
}

float DuplicateDetector::similarityThreshold() const
{
    QMutexLocker locker(&d->mutex);
    return d->similarityThresholdVal;
}

void DuplicateDetector::setSimilarityThreshold(float threshold)
{
    QMutexLocker locker(&d->mutex);
    if (d->similarityThresholdVal != threshold) {
        d->similarityThresholdVal = threshold;
        LOG_INFO("DuplicateDetector: Similarity threshold set to " << threshold);
    }
}

QStringList DuplicateDetector::supportedMethods() const
{
    return QStringList() << "hash" << "text"; // Add more as implemented
}

QString DuplicateDetector::activeMethod() const
{
    QMutexLocker locker(&d->mutex);
    return d->activeMethodStr;
}

void DuplicateDetector::setActiveMethod(const QString& method)
{
    QMutexLocker locker(&d->mutex);
    if (d->activeMethodStr != method && supportedMethods().contains(method)) {
        d->activeMethodStr = method;
        LOG_INFO("DuplicateDetector: Active method set to " << method);
    }
}

QString DuplicateDetector::calculateFileHash(const QString& filePath) const
{
    return d->calculateFileHash(filePath);
}

QString DuplicateDetector::fingerprintContent(Document* document) const
{
    // For now, delegate to the Private helper which handles text fingerprinting
    // A real implementation might also have image fingerprinting, metadata fingerprinting, etc.
    if (d->activeMethodStr == "text") {
        return d->fingerprintTextContent(document);
    }
    LOG_WARN("DuplicateDetector::fingerprintContent: Method '" << d->activeMethodStr << "' not implemented for content fingerprinting. Returning empty.");
    return QString(); // Placeholder
}

float DuplicateDetector::compareFingerprints(const QString& fp1, const QString& fp2) const
{
    // Defer to Private helpers based on active method
    if (d->activeMethodStr == "hash") {
        return d->compareHashes(fp1, fp2);
    } else if (d->activeMethodStr == "text") {
        return d->compareTextFingerprints(fp1, fp2);
    }
    LOG_WARN("DuplicateDetector::compareFingerprints: Method '" << d->activeMethodStr << "' not implemented for comparison. Returning 0.0.");
    return 0.0f; // Placeholder
}

} // namespace QuantilyxDoc