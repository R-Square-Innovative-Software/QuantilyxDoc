/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_DOCUMENTFACTORY_H
#define QUANTILYX_DOCUMENTFACTORY_H

#include <QObject>
#include <QString>
#include <QMap>
#include <memory>

namespace QuantilyxDoc {

class Document;
class Page;

/**
 * @brief Document factory class
 * 
 * Creates document instances based on file type. Uses a registry pattern
 * to allow format handlers to register themselves.
 */
class DocumentFactory : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Get singleton instance
     * @return DocumentFactory instance
     */
    static DocumentFactory& instance();
    
    /**
     * @brief Register a document type handler
     * @param extension File extension (without dot)
     * @param mimeType MIME type
     * @param creator Function that creates document instance
     */
    void registerDocumentType(const QString& extension, const QString& mimeType,
                             std::function<Document*()> creator);
    
    /**
     * @brief Create document for file
     * @param filePath Path to file
     * @param password Password for encrypted documents
     * @return Document instance or nullptr if creation failed
     */
    Document* createDocument(const QString& filePath, const QString& password = QString());
    
    /**
     * @brief Get supported file formats
     * @return List of supported file extensions
     */
    QStringList supportedExtensions() const;
    
    /**
     * @brief Get supported MIME types
     * @return List of supported MIME types
     */
    QStringList supportedMimeTypes() const;
    
    /**
     * @brief Check if file extension is supported
     * @param extension File extension (with or without dot)
     * @return true if supported
     */
    bool isExtensionSupported(const QString& extension) const;
    
    /**
     * @brief Check if MIME type is supported
     * @param mimeType MIME type
     * @return true if supported
     */
    bool isMimeTypeSupported(const QString& mimeType) const;
    
    /**
     * @brief Get document type from file path
     * @param filePath Path to file
     * @return DocumentType enum value
     */
    DocumentType documentTypeFromPath(const QString& filePath) const;
    
    /**
     * @brief Get document type from MIME type
     * @param mimeType MIME type
     * @return DocumentType enum value
     */
    DocumentType documentTypeFromMimeType(const QString& mimeType) const;
    
    /**
     * @brief Get document type from file extension
     * @param extension File extension (with or without dot)
     * @return DocumentType enum value
     */
    DocumentType documentTypeFromExtension(const QString& extension) const;
    
    /**
     * @brief Get filter string for file dialogs
     * @return Filter string suitable for QFileDialog
     */
    QString fileDialogFilter() const;

private:
    /**
     * @brief Private constructor for singleton
     */
    DocumentFactory();
    
    /**
     * @brief Disable copy constructor
     */
    DocumentFactory(const DocumentFactory&) = delete;
    
    /**
     * @brief Disable assignment operator
     */
    DocumentFactory& operator=(const DocumentFactory&) = delete;
    
    // Document creator function type
    using DocumentCreator = std::function<Document*()>;
    
    struct DocumentTypeRegistration {
        QString extension;
        QString mimeType;
        DocumentCreator creator;
    };
    
    class Private;
    std::unique_ptr<Private> d;
    static DocumentFactory* s_instance;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_DOCUMENTFACTORY_H