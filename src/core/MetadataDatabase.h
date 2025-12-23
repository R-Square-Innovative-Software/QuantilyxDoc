/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_METADATADATABASE_H
#define QUANTILYX_METADATADATABASE_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QVariantMap>
#include <QDateTime>
#include <QMutex>
#include <memory>

namespace QuantilyxDoc {

class Document; // Forward declaration

/**
 * @brief Structure holding metadata for a single document.
 */
struct DocumentMetadata {
    QString filePath;           // Canonical file path (key)
    QString title;              // Document title
    QString author;             // Author name
    QString subject;            // Subject
    QStringList keywords;       // List of keywords/tags
    QDateTime creationDate;     // Date created
    QDateTime modificationDate; // Date last modified
    QString format;             // Format string (e.g., "PDF 1.7", "EPUB 3.0")
    QString creator;            // Creating application
    QString producer;           // Processing application (e.g., PDF Producer)
    qint64 fileSize;            // Size in bytes
    int pageCount;              // Number of pages (if applicable)
    QString language;           // Language code (e.g., en_US)
    QString customFields;       // JSON string for any other arbitrary metadata fields
    QDateTime lastIndexed;      // Last time this entry was updated in the DB
};

/**
 * @brief Stores and queries document metadata and tags.
 * 
 * Uses an underlying database engine (e.g., SQLite) to provide fast search and retrieval
 * of document metadata and user-assigned tags.
 */
class MetadataDatabase : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit MetadataDatabase(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~MetadataDatabase() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global MetadataDatabase instance.
     */
    static MetadataDatabase& instance();

    /**
     * @brief Initialize the database.
     * Opens the database file and creates tables if they don't exist.
     * @param dbPath Path to the database file (e.g., SQLite).
     * @return True if initialization was successful.
     */
    bool initialize(const QString& dbPath = QString());

    /**
     * @brief Check if the database is initialized and ready.
     * @return True if ready.
     */
    bool isReady() const;

    /**
     * @brief Store or update metadata for a document in the database.
     * @param metadata The metadata structure to store.
     * @return True if the operation was successful.
     */
    bool storeMetadata(const DocumentMetadata& metadata);

    /**
     * @brief Retrieve metadata for a specific document from the database.
     * @param filePath Path to the document.
     * @return The metadata structure, or an invalid one if not found.
     */
    DocumentMetadata retrieveMetadata(const QString& filePath) const;

    /**
     * @brief Remove metadata for a document from the database.
     * @param filePath Path to the document.
     * @return True if the operation was successful.
     */
    bool removeMetadata(const QString& filePath);

    /**
     * @brief Query the database for documents matching certain criteria.
     * @param query A string or structured query (e.g., SQL WHERE clause, or a more abstract format).
     * @param limit Maximum number of results to return (0 for no limit).
     * @param offset Offset for pagination.
     * @return List of matching DocumentMetadata structures.
     */
    QList<DocumentMetadata> queryMetadata(const QString& query, int limit = 0, int offset = 0) const;

    /**
     * @brief Get a list of all unique tags present in the database.
     * @return List of tag strings.
     */
    QStringList getAllTags() const;

    /**
     * @brief Get a list of all unique authors present in the database.
     * @return List of author strings.
     */
    QStringList getAllAuthors() const;

    /**
     * @brief Get a list of all unique document formats present in the database.
     * @return List of format strings.
     */
    QStringList getAllFormats() const;

    /**
     * @brief Get the total number of documents indexed in the database.
     * @return Count of documents.
     */
    int documentCount() const;

    /**
     * @brief Get the total size of all documents referenced in the database.
     * @return Total size in bytes.
     */
    qint64 totalDocumentsSize() const;

    /**
     * @brief Update the database with metadata from a loaded Document object.
     * Extracts metadata from the Document and calls storeMetadata.
     * @param document The loaded document object.
     * @return True if the update was successful.
     */
    bool updateMetadataFromDocument(Document* document);

    /**
     * @brief Vacuum/optimize the database file.
     * This compacts the database and frees unused space (for SQLite).
     */
    void vacuum();

    /**
     * @brief Get the path to the database file.
     * @return Database file path string.
     */
    QString databasePath() const;

    /**
     * @brief Set the path to the database file.
     * @param path Database file path string.
     * @return True if the path was successfully set (does not initialize the database at the new path).
     */
    bool setDatabasePath(const QString& path);

signals:
    /**
     * @brief Emitted when metadata is successfully stored.
     * @param filePath Path of the document whose metadata was stored.
     */
    void metadataStored(const QString& filePath);

    /**
     * @brief Emitted when metadata is successfully removed.
     * @param filePath Path of the document whose metadata was removed.
     */
    void metadataRemoved(const QString& filePath);

    /**
     * @brief Emitted when the database content changes (metadata added/removed/updated).
     */
    void databaseContentChanged();

    /**
     * @brief Emitted when a query is executed successfully.
     * @param results The list of results returned by the query.
     */
    void queryExecuted(const QList<QuantilyxDoc::DocumentMetadata>& results);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_METADATADATABASE_H