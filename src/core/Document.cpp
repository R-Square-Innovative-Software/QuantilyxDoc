/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "Document.h"
#include "../utils/FileUtils.h"
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QMimeDatabase>
#include <QDebug>
#include <QDir>
namespace QuantilyxDoc {

class Document::Private {
public:
    Private() 
        : state(Unloaded)
        , modified(false)
        , currentPageIndex(0)
    {}
    QString filePath;
    QString title;
    QString author;
    QString subject;
    QStringList keywords;
    QDateTime creationDate;
    QDateTime modificationDate;
    State state;
    QString lastError;
    qint64 fileSize;
    bool modified;
    int currentPageIndex;
    QString formatVersion;
    bool locked;
    bool encrypted;
};

Document::Document(QObject* parent)
    : QObject(parent)
    , d(new Private())
{
    // Set default creation date to now
    d->creationDate = QDateTime::currentDateTime();
    d->modificationDate = d->creationDate;
    d->locked = false;
    d->encrypted = false;
}

Document::~Document()
{
}

void Document::close()
{
    setState(Unloaded);
    setFilePath(QString());
    emit closed();
}

QString Document::filePath() const
{
    return d->filePath;
}

QString Document::title() const
{
    return d->title.isEmpty() ? QFileInfo(d->filePath).fileName() : d->title;
}

QString Document::author() const
{
    return d->author;
}

QString Document::subject() const
{
    return d->subject;
}

QStringList Document::keywords() const
{
    return d->keywords;
}

QDateTime Document::creationDate() const
{
    return d->creationDate;
}

QDateTime Document::modificationDate() const
{
    return d->modificationDate;
}

int Document::currentPageIndex() const
{
    return d->currentPageIndex;
}

void Document::setCurrentPageIndex(int index)
{
    if (index != d->currentPageIndex) {
        d->currentPageIndex = index;
        emit currentPageChanged(index);
    }
}

bool Document::isModified() const
{
    return d->modified;
}

void Document::setModified(bool modified)
{
    if (d->modified != modified) {
        d->modified = modified;
        if (modified) {
            emit modified();
        }
    }
}

bool Document::isLocked() const
{
    return d->locked;
}

void Document::setLocked(bool locked)
{
    d->locked = locked;
}

bool Document::isEncrypted() const
{
    return d->encrypted;
}

void Document::setEncrypted(bool encrypted)
{
    d->encrypted = encrypted;
}

Document::State Document::state() const
{
    return d->state;
}

QString Document::lastError() const
{
    return d->lastError;
}

qint64 Document::fileSize() const
{
    return d->fileSize;
}

QString Document::formatVersion() const
{
    return d->formatVersion;
}

void Document::setFormatVersion(const QString& version)
{
    d->formatVersion = version;
}

bool Document::supportsFeature(const QString& feature) const
{
    Q_UNUSED(feature);
    return false; // Base implementation - no special features
}

QList<Annotation*> Document::annotations() const
{
    return QList<Annotation*>();
}

void Document::addAnnotation(Annotation* annotation)
{
    Q_UNUSED(annotation);
    // Base implementation does nothing
}

void Document::removeAnnotation(Annotation* annotation)
{
    Q_UNUSED(annotation);
    // Base implementation does nothing
}

bool Document::hasTableOfContents() const
{
    return false;
}

QVariantList Document::tableOfContents() const
{
    return QVariantList();
}

QStringList Document::bookmarks() const
{
    return QStringList();
}

void Document::addBookmark(const QString& name, int pageIndex)
{
    Q_UNUSED(name);
    Q_UNUSED(pageIndex);
    // Base implementation does nothing
}

void Document::removeBookmark(const QString& name)
{
    Q_UNUSED(name);
    // Base implementation does nothing
}

QList<int> Document::search(const QString& text, bool caseSensitive, bool wholeWords) const
{
    Q_UNUSED(text);
    Q_UNUSED(caseSensitive);
    Q_UNUSED(wholeWords);
    return QList<int>();
}

void Document::setState(State state)
{
    d->state = state;
}

void Document::setLastError(const QString& error)
{
    d->lastError = error;
}

void Document::setFilePath(const QString& path)
{
    d->filePath = path;
    
    // Update file info if path is not empty
    if (!path.isEmpty()) {
        QFileInfo fileInfo(path);
        d->fileSize = fileInfo.size();
        
        // Extract title from filename if not set
        if (d->title.isEmpty()) {
            d->title = fileInfo.baseName();
        }
        
        // Update modification date
        d->modificationDate = fileInfo.lastModified();
    } else {
        d->fileSize = 0;
    }
}

void Document::setTitle(const QString& title)
{
    d->title = title;
}

void Document::setAuthor(const QString& author)
{
    d->author = author;
}

void Document::setSubject(const QString& subject)
{
    d->subject = subject;
}

void Document::setKeywords(const QStringList& keywords)
{
    d->keywords = keywords;
}

void Document::setCreationDate(const QDateTime& date)
{
    d->creationDate = date;
}

void Document::setModificationDate(const QDateTime& date)
{
    d->modificationDate = date;
}

} // namespace QuantilyxDoc