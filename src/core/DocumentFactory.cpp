/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "DocumentFactory.h"
#include "Document.h"
#include "../formats/pdf/PdfDocument.h"
#include "../formats/epub/EpubDocument.h"
#include "../formats/djvu/DjvuDocument.h"
#include "../formats/comic/CbzDocument.h"
#include "../formats/comic/CbrDocument.h"
#include "../formats/postscript/PsDocument.h"
#include "../formats/xps/XpsDocument.h"
#include "../formats/chm/ChmDocument.h"
#include "../formats/markdown/MdDocument.h"
#include "../formats/fictionbook/Fb2Document.h"
#include "../formats/mobi/MobiDocument.h"
#include "../formats/image/ImageDocument.h"
#include "../formats/cad/DxfDocument.h"
#include "../formats/office/OdtDocument.h"
#include "../formats/office/DocxDocument.h"
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

namespace QuantilyxDoc {

class DocumentFactory::Private {
public:
    Private() {}
    QMap<QString, DocumentTypeRegistration> extensionRegistry;
    QMap<QString, DocumentTypeRegistration> mimeRegistry;
};

DocumentFactory* DocumentFactory::s_instance = nullptr;

DocumentFactory::DocumentFactory()
    : d(new Private())
{
    // Register built-in document types
    registerDocumentType("pdf", "application/pdf", []() { return new PdfDocument(); });
    registerDocumentType("epub", "application/epub+zip", []() { return new EpubDocument(); });
    registerDocumentType("djvu", "image/vnd.djvu", []() { return new DjvuDocument(); });
    registerDocumentType("djv", "image/vnd.djvu", []() { return new DjvuDocument(); });
    registerDocumentType("cbz", "application/vnd.comicbook+zip", []() { return new CbzDocument(); });
    registerDocumentType("cbr", "application/vnd.comicbook+rar", []() { return new CbrDocument(); });
    registerDocumentType("ps", "application/postscript", []() { return new PsDocument(); });
    registerDocumentType("eps", "application/postscript", []() { return new PsDocument(); });
    registerDocumentType("xps", "application/vnd.ms-xpsdocument", []() { return new XpsDocument(); });
    registerDocumentType("chm", "application/vnd.ms-htmlhelp", []() { return new ChmDocument(); });
    registerDocumentType("md", "text/markdown", []() { return new MdDocument(); });
    registerDocumentType("markdown", "text/markdown", []() { return new MdDocument(); });
    registerDocumentType("fb2", "application/fb2+zip", []() { return new Fb2Document(); });
    registerDocumentType("mobi", "application/x-mobipocket-ebook", []() { return new MobiDocument(); });
    registerDocumentType("jpg", "image/jpeg", []() { return new ImageDocument(); });
    registerDocumentType("jpeg", "image/jpeg", []() { return new ImageDocument(); });
    registerDocumentType("png", "image/png", []() { return new ImageDocument(); });
    registerDocumentType("gif", "image/gif", []() { return new ImageDocument(); });
    registerDocumentType("bmp", "image/bmp", []() { return new ImageDocument(); });
    registerDocumentType("tiff", "image/tiff", []() { return new ImageDocument(); });
    registerDocumentType("tif", "image/tiff", []() { return new ImageDocument(); });
    registerDocumentType("webp", "image/webp", []() { return new ImageDocument(); });
    registerDocumentType("dxf", "application/dxf", []() { return new DxfDocument(); });
    registerDocumentType("dwg", "application/acad", []() { return new DwgDocument(); });
    registerDocumentType("odt", "application/vnd.oasis.opendocument.text", []() { return new OdtDocument(); });
    registerDocumentType("docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document", []() { return new DocxDocument(); });
}

DocumentFactory& DocumentFactory::instance()
{
    if (!s_instance) {
        s_instance = new DocumentFactory();
    }
    return *s_instance;
}

void DocumentFactory::registerDocumentType(const QString& extension, const QString& mimeType,
                                          std::function<Document*()> creator)
{
    QString ext = extension.startsWith('.') ? extension : ("." + extension);
    
    DocumentTypeRegistration reg;
    reg.extension = ext;
    reg.mimeType = mimeType;
    reg.creator = creator;
    
    d->extensionRegistry.insert(ext, reg);
    d->mimeRegistry.insert(mimeType, reg);
}

Document* DocumentFactory::createDocument(const QString& filePath, const QString& password)
{
    if (filePath.isEmpty()) {
        return nullptr;
    }
    
    QFileInfo fileInfo(filePath);
    QString extension = "." + fileInfo.suffix().toLower();
    
    // Find registration by extension
    if (d->extensionRegistry.contains(extension)) {
        Document* doc = d->extensionRegistry[extension].creator();
        if (doc) {
            doc->setParent(this);
            bool success = doc->load(filePath, password);
            if (!success) {
                delete doc;
                return nullptr;
            }
            return doc;
        }
    }
    
    // Try MIME type detection as fallback
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(filePath);
    
    if (d->mimeRegistry.contains(mimeType.name())) {
        Document* doc = d->mimeRegistry[mimeType.name()].creator();
        if (doc) {
            doc->setParent(this);
            bool success = doc->load(filePath, password);
            if (!success) {
                delete doc;
                return nullptr;
            }
            return doc;
        }
    }
    
    // No matching handler found
    return nullptr;
}

QStringList DocumentFactory::supportedExtensions() const
{
    QStringList extensions;
    extensions.reserve(d->extensionRegistry.size());
    
    for (auto it = d->extensionRegistry.begin(); it != d->extensionRegistry.end(); ++it) {
        extensions.append(it->extension);
    }
    
    std::sort(extensions.begin(), extensions.end());
    return extensions;
}

QStringList DocumentFactory::supportedMimeTypes() const
{
    QStringList mimeTypes;
    mimeTypes.reserve(d->mimeRegistry.size());
    
    for (auto it = d->mimeRegistry.begin(); it != d->mimeRegistry.end(); ++it) {
        mimeTypes.append(it->mimeType);
    }
    
    std::sort(mimeTypes.begin(), mimeTypes.end());
    return mimeTypes;
}

bool DocumentFactory::isExtensionSupported(const QString& extension) const
{
    QString ext = extension.startsWith('.') ? extension : ("." + extension);
    return d->extensionRegistry.contains(ext);
}

bool DocumentFactory::isMimeTypeSupported(const QString& mimeType) const
{
    return d->mimeRegistry.contains(mimeType);
}

DocumentType DocumentFactory::documentTypeFromPath(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString extension = "." + fileInfo.suffix().toLower();
    
    if (d->extensionRegistry.contains(extension)) {
        return d->extensionRegistry[extension].creator()->type();
    }
    
    // Fallback to MIME type
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(filePath);
    
    if (d->mimeRegistry.contains(mimeType.name())) {
        return d->mimeRegistry[mimeType.name()].creator()->type();
    }
    
    return DocumentType::Unknown;
}

DocumentType DocumentFactory::documentTypeFromMimeType(const QString& mimeType) const
{
    if (d->mimeRegistry.contains(mimeType)) {
        return d->mimeRegistry[mimeType].creator()->type();
    }
    return DocumentType::Unknown;
}

DocumentType DocumentFactory::documentTypeFromExtension(const QString& extension) const
{
    QString ext = extension.startsWith('.') ? extension : ("." + extension);
    if (d->extensionRegistry.contains(ext)) {
        return d->extensionRegistry[ext].creator()->type();
    }
    return DocumentType::Unknown;
}

QString DocumentFactory::fileDialogFilter() const
{
    QStringList filters;
    
    // Add "All Supported Files" filter
    QStringList extensions;
    for (auto it = d->extensionRegistry.begin(); it != d->extensionRegistry.end(); ++it) {
        extensions.append("*" + it->extension);
    }
    std::sort(extensions.begin(), extensions.end());
    filters.append(QString("All Supported Files (%1)").arg(extensions.join(" ")));
    
    // Add individual format filters
    QMap<QString, QStringList> formatExtensions;
    
    // PDF
    formatExtensions["PDF Files"].append("*.pdf");
    // EPUB
    formatExtensions["EPUB Files"].append("*.epub");
    // DjVu
    formatExtensions["DjVu Files"].append("*.djvu");
    formatExtensions["DjVu Files"].append("*.djv");
    // Comic books
    formatExtensions["Comic Books"].append("*.cbz");
    formatExtensions["Comic Books"].append("*.cbr");
    // PostScript
    formatExtensions["PostScript Files"].append("*.ps");
    formatExtensions["PostScript Files"].append("*.eps");
    // XPS
    formatExtensions["XPS Files"].append("*.xps");
    // CHM
    formatExtensions["CHM Files"].append("*.chm");
    // Markdown
    formatExtensions["Markdown Files"].append("*.md");
    formatExtensions["Markdown Files"].append("*.markdown");
    // FictionBook
    formatExtensions["FictionBook Files"].append("*.fb2");
    // Mobi
    formatExtensions["Mobi Files"].append("*.mobi");
    // Images
    QStringList imageExts;
    imageExts << "*.jpg" << "*.jpeg" << "*.png" << "*.gif" << "*.bmp" << "*.tiff" << "*.tif" << "*.webp";
    formatExtensions["Image Files"] = imageExts;
    // CAD
    formatExtensions["2D CAD Files"].append("*.dxf");
    formatExtensions["2D CAD Files"].append("*.dwg");
    // Office
    formatExtensions["Office Documents"].append("*.odt");
    formatExtensions["Office Documents"].append("*.docx");
    
    // Add format filters
    for (auto it = formatExtensions.begin(); it != formatExtensions.end(); ++it) {
        filters.append(QString("%1 (%2)").arg(it.key(), it.value().join(" ")));
    }
    
    // Add "All Files" filter
    filters.append("All Files (*)");
    
    return filters.join(";;");
}

} // namespace QuantilyxDoc