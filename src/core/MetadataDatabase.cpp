/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "MetadataDatabase.h"
#include "Document.h" // For updateMetadataFromDocument
#include "Logger.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace QuantilyxDoc {

class MetadataDatabase::Private {
public:
    Private(MetadataDatabase* q_ptr)
        : q(q_ptr), ready(false) {}

    MetadataDatabase* q;
    mutable QMutex mutex; // Protect access to the QSqlDatabase connection
    bool ready;
    QString dbPathStr;
    QSqlDatabase sqlDb;

    // Helper to create the necessary tables
    bool createTables() {
        // Use a transaction for efficiency
        sqlDb.transaction();

        // Main metadata table
        QString createMetadataTable = R"(
            CREATE TABLE IF NOT EXISTS document_metadata (
                file_path TEXT PRIMARY KEY,
                title TEXT,
                author TEXT,
                subject TEXT,
                keywords TEXT, -- Store as JSON array string
                creation_date TEXT, -- ISO datetime string
                modification_date TEXT, -- ISO datetime string
                format TEXT,
                creator TEXT,
                producer TEXT,
                file_size INTEGER,
                page_count INTEGER,
                language TEXT,
                custom_fields TEXT, -- JSON string for arbitrary fields
                last_indexed TEXT -- ISO datetime string
            );
        )";

        // Tags table (for many-to-many relationship with documents)
        QString createTagsTable = R"(
            CREATE TABLE IF NOT EXISTS tags (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                tag_name TEXT UNIQUE NOT NULL
            );
        )";

        // Junction table for document-tag association
        QString createDocTagsTable = R"(
            CREATE TABLE IF NOT EXISTS document_tags (
                doc_file_path TEXT,
                tag_id INTEGER,
                FOREIGN KEY(doc_file_path) REFERENCES document_metadata(file_path) ON DELETE CASCADE,
                FOREIGN KEY(tag_id) REFERENCES tags(id) ON DELETE CASCADE,
                PRIMARY KEY(doc_file_path, tag_id)
            );
        )";

        QSqlQuery query(sqlDb);
        if (!query.exec(createMetadataTable)) {
            LOG_ERROR("MetadataDatabase: Failed to create metadata table: " << query.lastError().text());
            sqlDb.rollback();
            return false;
        }
        if (!query.exec(createTagsTable)) {
            LOG_ERROR("MetadataDatabase: Failed to create tags table: " << query.lastError().text());
            sqlDb.rollback();
            return false;
        }
        if (!query.exec(createDocTagsTable)) {
            LOG_ERROR("MetadataDatabase: Failed to create document_tags table: " << query.lastError().text());
            sqlDb.rollback();
            return false;
        }

        // Create indexes for faster queries
        QString createPathIndex = "CREATE INDEX IF NOT EXISTS idx_doc_path ON document_metadata(file_path);";
        QString createAuthorIndex = "CREATE INDEX IF NOT EXISTS idx_author ON document_metadata(author);";
        QString createFormatIndex = "CREATE INDEX IF NOT EXISTS idx_format ON document_metadata(format);";
        QString createKeywordIndex = "CREATE INDEX IF NOT EXISTS idx_keywords ON document_metadata(keywords);"; // Might be slow on JSON, consider full-text search

        for (const QString& indexSql : {createPathIndex, createAuthorIndex, createFormatIndex, createKeywordIndex}) {
            if (!query.exec(indexSql)) {
                LOG_WARN("MetadataDatabase: Failed to create index: " << query.lastError().text()); // Non-fatal
            }
        }

        sqlDb.commit();
        LOG_DEBUG("MetadataDatabase: Tables created/verified successfully.");
        return true;
    }

    // Helper to convert DocumentMetadata to SQL query values
    QVariantMap metadataToSqlValues(const DocumentMetadata& metadata) const {
        QVariantMap values;
        values[":file_path"] = metadata.filePath;
        values[":title"] = metadata.title;
        values[":author"] = metadata.author;
        values[":subject"] = metadata.subject;
        values[":keywords"] = QJsonDocument(QJsonArray::fromStringList(metadata.keywords)).toJson(QJsonDocument::Compact);
        values[":creation_date"] = metadata.creationDate.toString(Qt::ISODateWithMs);
        values[":modification_date"] = metadata.modificationDate.toString(Qt::ISODateWithMs);
        values[":format"] = metadata.format;
        values[":creator"] = metadata.creator;
        values[":producer"] = metadata.producer;
        values[":file_size"] = metadata.fileSize;
        values[":page_count"] = metadata.pageCount;
        values[":language"] = metadata.language;
        values[":custom_fields"] = metadata.customFields;
        values[":last_indexed"] = metadata.lastIndexed.toString(Qt::ISODateWithMs);
        return values;
    }

    // Helper to convert SQL query results to DocumentMetadata
    DocumentMetadata sqlValuesToMetadata(const QSqlQuery& query) const {
        DocumentMetadata metadata;
        metadata.filePath = query.value("file_path").toString();
        metadata.title = query.value("title").toString();
        metadata.author = query.value("author").toString();
        metadata.subject = query.value("subject").toString();

        QString keywordsJson = query.value("keywords").toString();
        if (!keywordsJson.isEmpty()) {
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(keywordsJson.toUtf8(), &error);
            if (error.error == QJsonParseError::NoError && doc.isArray()) {
                QStringList keywords;
                for (const auto& value : doc.array()) {
                    if (value.isString()) {
                        keywords.append(value.toString());
                    }
                }
                metadata.keywords = keywords;
            }
        }

        metadata.creationDate = QDateTime::fromString(query.value("creation_date").toString(), Qt::ISODateWithMs);
        metadata.modificationDate = QDateTime::fromString(query.value("modification_date").toString(), Qt::ISODateWithMs);
        metadata.format = query.value("format").toString();
        metadata.creator = query.value("creator").toString();
        metadata.producer = query.value("producer").toString();
        metadata.fileSize = query.value("file_size").toLongLong();
        metadata.pageCount = query.value("page_count").toInt();
        metadata.language = query.value("language").toString();
        metadata.customFields = query.value("custom_fields").toString();
        metadata.lastIndexed = QDateTime::fromString(query.value("last_indexed").toString(), Qt::ISODateWithMs);
        return metadata;
    }
};

// Static instance pointer
MetadataDatabase* MetadataDatabase::s_instance = nullptr;

MetadataDatabase& MetadataDatabase::instance()
{
    if (!s_instance) {
        s_instance = new MetadataDatabase();
    }
    return *s_instance;
}

MetadataDatabase::MetadataDatabase(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    LOG_INFO("MetadataDatabase created.");
}

MetadataDatabase::~MetadataDatabase()
{
    if (d->sqlDb.isOpen()) {
        d->sqlDb.close();
    }
    LOG_INFO("MetadataDatabase destroyed.");
}

bool MetadataDatabase::initialize(const QString& dbPath)
{
    QMutexLocker locker(&d->mutex);

    if (d->ready) {
        LOG_WARN("MetadataDatabase::initialize: Already initialized.");
        return true;
    }

    QString path = dbPath;
    if (path.isEmpty()) {
        // Use a default path, e.g., in the application data directory
        path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/quantilyxdoc_metadata.db";
        QDir().mkpath(QFileInfo(path).absolutePath()); // Ensure directory exists
    }

    // Add a unique connection name to avoid conflicts with other parts of the application using QSqlDatabase
    QString connectionName = "metadata_db_connection";
    d->sqlDb = QSqlDatabase::addDatabase("QSQLITE", connectionName); // Use SQLite driver
    d->sqlDb.setDatabaseName(path);
    // d->sqlDb.setUserName(...); // Not typically needed for SQLite
    // d->sqlDb.setPassword(...); // Not typically needed for SQLite
    d->sqlDb.setConnectOptions("QSQLITE_OPEN_URI"); // Allow URI filenames

    if (!d->sqlDb.open()) {
        LOG_ERROR("MetadataDatabase: Failed to open database: " << d->sqlDb.lastError().text());
        d->ready = false;
        return false;
    }

    if (!d->createTables()) {
        LOG_ERROR("MetadataDatabase: Failed to create tables.");
        d->ready = false;
        d->sqlDb.close();
        return false;
    }

    d->dbPathStr = path;
    d->ready = true;
    LOG_INFO("MetadataDatabase: Initialized successfully at: " << path);
    return true;
}

bool MetadataDatabase::isReady() const
{
    QMutexLocker locker(&d->mutex);
    return d->ready;
}

bool MetadataDatabase::storeMetadata(const DocumentMetadata& metadata)
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::storeMetadata: Database is not ready.");
        return false;
    }

    QMutexLocker locker(&d->mutex);

    QSqlQuery query(d->sqlDb);
    query.prepare(R"(
        INSERT OR REPLACE INTO document_metadata
        (file_path, title, author, subject, keywords, creation_date, modification_date, format, creator, producer, file_size, page_count, language, custom_fields, last_indexed)
        VALUES (:file_path, :title, :author, :subject, :keywords, :creation_date, :modification_date, :format, :creator, :producer, :file_size, :page_count, :language, :custom_fields, :last_indexed)
    )");

    QVariantMap values = d->metadataToSqlValues(metadata);
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        query.bindValue(it.key(), it.value());
    }

    if (!query.exec()) {
        LOG_ERROR("MetadataDatabase: Failed to store metadata for " << metadata.filePath << ": " << query.lastError().text());
        return false;
    }

    LOG_DEBUG("MetadataDatabase: Stored metadata for: " << metadata.filePath);
    emit metadataStored(metadata.filePath);
    emit databaseContentChanged();
    return true;
}

DocumentMetadata MetadataDatabase::retrieveMetadata(const QString& filePath) const
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::retrieveMetadata: Database is not ready.");
        return DocumentMetadata(); // Return invalid metadata
    }

    QMutexLocker locker(&d->mutex);

    QSqlQuery query(d->sqlDb);
    query.prepare("SELECT * FROM document_metadata WHERE file_path = :file_path;");
    query.bindValue(":file_path", filePath);

    if (!query.exec() || !query.next()) {
        if (query.lastError().isValid()) {
            LOG_ERROR("MetadataDatabase: Query failed for " << filePath << ": " << query.lastError().text());
        } else {
            LOG_DEBUG("MetadataDatabase: No metadata found for: " << filePath);
        }
        return DocumentMetadata(); // Return invalid metadata
    }

    DocumentMetadata metadata = d->sqlValuesToMetadata(query);
    LOG_DEBUG("MetadataDatabase: Retrieved metadata for: " << filePath);
    return metadata;
}

bool MetadataDatabase::removeMetadata(const QString& filePath)
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::removeMetadata: Database is not ready.");
        return false;
    }

    QMutexLocker locker(&d->mutex);

    // Deleting from document_metadata will cascade delete from document_tags due to foreign key constraint
    QSqlQuery query(d->sqlDb);
    query.prepare("DELETE FROM document_metadata WHERE file_path = :file_path;");
    query.bindValue(":file_path", filePath);

    if (!query.exec()) {
        LOG_ERROR("MetadataDatabase: Failed to remove metadata for " << filePath << ": " << query.lastError().text());
        return false;
    }

    if (query.numRowsAffected() > 0) {
        LOG_DEBUG("MetadataDatabase: Removed metadata for: " << filePath);
        emit metadataRemoved(filePath);
        emit databaseContentChanged();
    } else {
        LOG_DEBUG("MetadataDatabase: No metadata entry found to remove for: " << filePath);
    }
    return true;
}

QList<DocumentMetadata> MetadataDatabase::queryMetadata(const QString& queryString, int limit, int offset) const
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::queryMeta Database is not ready.");
        return {}; // Return empty list
    }

    QMutexLocker locker(&d->mutex);

    // WARNING: Directly inserting the queryString into the SQL is DANGEROUS if it comes from user input.
    // This example assumes queryString is a trusted, pre-sanitized internal query string or a specific field name/value pair.
    // A safer approach would be to build the WHERE clause programmatically based on a structured query object.
    // Example of safer query building:
    // QString baseQuery = "SELECT * FROM document_metadata ";
    // QStringList conditions;
    // QVariantMap params; // Use bound values for user-provided data
    // if (!author.isEmpty()) {
    //     conditions.append("author = :author");
    //     params[":author"] = author;
    // }
    // if (!titleKeyword.isEmpty()) {
    //     conditions.append("title LIKE :title");
    //     params[":title"] = "%" + titleKeyword + "%";
    // }
    // QString fullQuery = baseQuery;
    // if (!conditions.isEmpty()) {
    //     fullQuery += "WHERE " + conditions.join(" AND ");
    // }
    // fullQuery += " LIMIT :limit OFFSET :offset;";
    // query.prepare(fullQuery);
    // for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
    //     query.bindValue(it.key(), it.value());
    // }
    // query.bindValue(":limit", limit);
    // query.bindValue(":offset", offset);

    // For now, let's use a placeholder query that just selects everything with limit/offset
    QString fullQuery = "SELECT * FROM document_metadata "; // Add WHERE clause based on queryString if it's a structured query
    if (limit > 0) {
        fullQuery += QString("LIMIT %1 ").arg(limit);
        if (offset > 0) {
            fullQuery += QString("OFFSET %1 ").arg(offset);
        }
    }
    fullQuery += ";"; // Ensure query ends

    QSqlQuery query(d->sqlDb);
    // For a real implementation, queryString should be used to build the WHERE clause safely.
    // query.prepare(queryString); // DO NOT DO THIS WITH USER INPUT
    query.prepare(fullQuery); // Use the built query

    if (!query.exec()) {
        LOG_ERROR("MetadataDatabase: Query failed: " << query.lastError().text() << ", Query: " << fullQuery);
        return {}; // Return empty list on error
    }

    QList<DocumentMetadata> results;
    while (query.next()) {
        results.append(d->sqlValuesToMetadata(query));
    }

    LOG_DEBUG("MetadataDatabase: Query returned " << results.size() << " results.");
    emit queryExecuted(results);
    return results;
}

QStringList MetadataDatabase::getAllTags() const
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::getAllTags: Database is not ready.");
        return {}; // Return empty list
    }

    QMutexLocker locker(&d->mutex);

    QSqlQuery query(d->sqlDb);
    query.prepare("SELECT DISTINCT tag_name FROM tags ORDER BY tag_name ASC;");

    if (!query.exec()) {
        LOG_ERROR("MetadataDatabase: Failed to get all tags: " << query.lastError().text());
        return {}; // Return empty list on error
    }

    QStringList tags;
    while (query.next()) {
        tags.append(query.value("tag_name").toString());
    }

    LOG_DEBUG("MetadataDatabase: Retrieved " << tags.size() << " unique tags.");
    return tags;
}

QStringList MetadataDatabase::getAllAuthors() const
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::getAllAuthors: Database is not ready.");
        return {}; // Return empty list
    }

    QMutexLocker locker(&d->mutex);

    QSqlQuery query(d->sqlDb);
    query.prepare("SELECT DISTINCT author FROM document_metadata WHERE author IS NOT NULL AND author != '' ORDER BY author ASC;");

    if (!query.exec()) {
        LOG_ERROR("MetadataDatabase: Failed to get all authors: " << query.lastError().text());
        return {}; // Return empty list on error
    }

    QStringList authors;
    while (query.next()) {
        authors.append(query.value("author").toString());
    }

    LOG_DEBUG("MetadataDatabase: Retrieved " << authors.size() << " unique authors.");
    return authors;
}

QStringList MetadataDatabase::getAllFormats() const
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::getAllFormats: Database is not ready.");
        return {}; // Return empty list
    }

    QMutexLocker locker(&d->mutex);

    QSqlQuery query(d->sqlDb);
    query.prepare("SELECT DISTINCT format FROM document_metadata WHERE format IS NOT NULL AND format != '' ORDER BY format ASC;");

    if (!query.exec()) {
        LOG_ERROR("MetadataDatabase: Failed to get all formats: " << query.lastError().text());
        return {}; // Return empty list on error
    }

    QStringList formats;
    while (query.next()) {
        formats.append(query.value("format").toString());
    }

    LOG_DEBUG("MetadataDatabase: Retrieved " << formats.size() << " unique formats.");
    return formats;
}

int MetadataDatabase::documentCount() const
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::documentCount: Database is not ready.");
        return 0; // Return 0 on error
    }

    QMutexLocker locker(&d->mutex);

    QSqlQuery query(d->sqlDb);
    query.prepare("SELECT COUNT(*) AS count FROM document_metadata;");

    if (!query.exec() || !query.next()) {
        LOG_ERROR("MetadataDatabase: Failed to count documents: " << (query.lastError().isValid() ? query.lastError().text() : "No result"));
        return 0; // Return 0 on error
    }

    int count = query.value("count").toInt();
    LOG_DEBUG("MetadataDatabase: Total documents indexed: " << count);
    return count;
}

qint64 MetadataDatabase::totalDocumentsSize() const
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::totalDocumentsSize: Database is not ready.");
        return 0; // Return 0 on error
    }

    QMutexLocker locker(&d->mutex);

    QSqlQuery query(d->sqlDb);
    query.prepare("SELECT SUM(file_size) AS total_size FROM document_metadata;");

    if (!query.exec() || !query.next()) {
        LOG_ERROR("MetadataDatabase: Failed to calculate total size: " << (query.lastError().isValid() ? query.lastError().text() : "No result"));
        return 0; // Return 0 on error
    }

    qint64 totalSize = query.value("total_size").toLongLong();
    LOG_DEBUG("MetadataDatabase: Total size of indexed documents: " << totalSize << " bytes.");
    return totalSize;
}

bool MetadataDatabase::updateMetadataFromDocument(Document* document)
{
    if (!document) {
        LOG_ERROR("MetadataDatabase::updateMetadataFromDocument: Null document provided.");
        return false;
    }

    // Extract metadata from the Document object.
    // This requires the Document class to have methods for all metadata fields.
    DocumentMetadata metadata;
    metadata.filePath = document->filePath(); // Assuming Document has this
    metadata.title = document->title(); // Assuming Document has this
    metadata.author = document->author(); // Assuming Document has this
    metadata.subject = document->subject(); // Assuming Document has this
    metadata.keywords = document->keywords(); // Assuming Document has this
    metadata.creationDate = document->creationDate(); // Assuming Document has this
    metadata.modificationDate = document->modificationDate(); // Assuming Document has this
    metadata.format = document->formatVersion(); // Assuming Document has this
    metadata.creator = document->creator(); // Assuming Document has this (or get from specific format handlers)
    metadata.producer = document->producer(); // Assuming Document has this (or get from specific format handlers)
    metadata.fileSize = document->fileSize(); // Assuming Document has this
    metadata.pageCount = document->pageCount(); // Assuming Document has this
    metadata.language = document->language(); // Assuming Document has this
    // metadata.customFields = ...; // Could store format-specific fields as JSON
    metadata.lastIndexed = QDateTime::currentDateTime();

    return storeMetadata(metadata);
}

void MetadataDatabase::vacuum()
{
    if (!isReady()) {
        LOG_ERROR("MetadataDatabase::vacuum: Database is not ready.");
        return;
    }

    QMutexLocker locker(&d->mutex);

    QSqlQuery query(d->sqlDb);
    if (!query.exec("VACUUM;")) {
        LOG_ERROR("MetadataDatabase: Vacuum operation failed: " << query.lastError().text());
    } else {
        LOG_INFO("MetadataDatabase: Vacuum operation completed.");
    }
}

QString MetadataDatabase::databasePath() const
{
    QMutexLocker locker(&d->mutex);
    return d->dbPathStr;
}

bool MetadataDatabase::setDatabasePath(const QString& path)
{
    // This function sets the *desired* path but doesn't change the currently open database connection.
    // To use a new path, the database must be closed and reopened with initialize(newPath).
    // For now, just store the path if it's different.
    QMutexLocker locker(&d->mutex);
    if (d->dbPathStr != path) {
        d->dbPathStr = path;
        LOG_INFO("MetadataDatabase: Database path set to: " << path << " (Reinitialize to use).");
        return true;
    }
    return false; // Path was already set
}

} // namespace QuantilyxDoc