/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "Clipboard.h"
#include "Logger.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QImage>
#include <QBuffer>
#include <QUrl>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QDebug>

namespace QuantilyxDoc {

class Clipboard::Private {
public:
    Private(Clipboard* q_ptr)
        : q(q_ptr),
          historyEnabled(true),
          maxHistorySizeVal(20) {}

    Clipboard* q;
    mutable QMutex mutex; // Protect access to history list and system clipboard connection
    QList<HistoryEntry> history;
    bool historyEnabled;
    int maxHistorySizeVal;

    // Helper to get system clipboard
    QClipboard* getSystemClipboard() const {
        return QApplication::clipboard();
    }

    // Helper to add current system clipboard content to history
    void addToHistoryIfNeeded() {
        if (!historyEnabled) return;

        const QMimeData* sysData = getSystemClipboard()->mimeData();
        if (!sysData) return;

        // Check if this exact data is already the most recent in history
        if (!history.isEmpty() && dataEquals(history.last().data, sysData)) {
             LOG_DEBUG("Current clipboard data matches last history entry. Skipping.");
             return;
        }

        HistoryEntry entry;
        entry.data = cloneMimeData(sysData);
        entry.timestamp = QDateTime::currentDateTime();
        entry.dataType = sysData->formats().isEmpty() ? "unknown" : sysData->formats().first();
        entry.previewText = generatePreviewText(sysData);
        entry.dataSize = approximateDataSize(sysData);

        history.append(entry);
        LOG_DEBUG("Added clipboard content to history. Type: " << entry.dataType << ", Size: " << entry.dataSize << " bytes.");

        // Maintain max size
        while (history.size() > maxHistorySizeVal) {
            HistoryEntry oldEntry = history.takeFirst(); // Remove oldest
            LOG_DEBUG("Evicted old clipboard history entry.");
        }

        emit q->historyChanged();
        emit q->historyItemAdded(entry);
    }

    // Helper to check if two QMimeData objects are equivalent
    bool dataEquals(const QMimeData* a, const QMimeData* b) const {
        if (!a && !b) return true;
        if (!a || !b) return false;
        // Compare formats and primary content (simplified check)
        return a->formats() == b->formats() &&
               a->text() == b->text() &&
               a->html() == b->html() &&
               a->hasImage() == b->hasImage(); // Could be more thorough
    }

    // Helper to deep clone QMimeData
    QMimeData* cloneMimeData(const QMimeData* original) const {
        if (!original) return nullptr;

        QMimeData* clone = new QMimeData();
        // Copy standard formats
        if (original->hasText()) clone->setText(original->text());
        if (original->hasHtml()) clone->setHtml(original->html());
        if (original->hasImage()) clone->setImageData(original->imageData());
        if (original->hasUrls()) clone->setUrls(original->urls());
        if (original->hasColor()) clone->setColorData(original->colorData());

        // Copy custom formats
        for (const QString& format : original->formats()) {
            if (!clone->hasFormat(format)) { // Avoid overwriting standard ones set above
                clone->setData(format, original->data(format));
            }
        }
        return clone;
    }

    // Helper to generate a short preview text
    QString generatePreviewText(const QMimeData* data) const {
        if (!data) return QString();

        if (data->hasText()) {
            QString text = data->text();
            if (text.length() > 50) text = text.left(50) + "...";
            return text;
        }
        if (data->hasHtml()) {
            QString html = data->html();
            // Strip HTML tags for preview
            html.remove(QRegularExpression("<[^>]*>"));
            if (html.length() > 50) html = html.left(50) + "...";
            return html;
        }
        if (data->hasImage()) {
            return "[Image]";
        }
        if (data->hasUrls()) {
            auto urls = data->urls();
            if (!urls.isEmpty()) {
                return "[URL: " + urls.first().toString() + "]";
            }
        }
        // Default to first format name if no specific content recognized
        auto formats = data->formats();
        return formats.isEmpty() ? "[Unknown Data]" : "[" + formats.first() + "]";
    }

    // Helper to estimate data size (very rough approximation)
    qint64 approximateDataSize(const QMimeData* data) const {
        if (!data) return 0;
        qint64 size = 0;
        if (data->hasText()) size += data->text().length() * sizeof(QChar);
        if (data->hasHtml()) size += data->html().length() * sizeof(QChar);
        if (data->hasImage()) {
            QImage img = qvariant_cast<QImage>(data->imageData());
            if (!img.isNull()) size += img.sizeInBytes();
        }
        size += data->urls().size() * 100; // Rough estimate per URL
        // Add sizes for other formats if needed
        return size;
    }

    // Helper to sanitize HTML content
    QString sanitizeHtml(const QString& html) const {
        QString sanitized = html;
        // Remove potentially dangerous tags/scripts (basic example)
        sanitized.remove(QRegularExpression("<script[^>]*>.*?</script>", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
        sanitized.remove(QRegularExpression("<iframe[^>]*>.*?</iframe>", QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption));
        // Add more sanitization rules as needed
        return sanitized;
    }
};

// Static instance pointer
Clipboard* Clipboard::s_instance = nullptr;

Clipboard& Clipboard::instance()
{
    if (!s_instance) {
        s_instance = new Clipboard();
    }
    return *s_instance;
}

Clipboard::Clipboard(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    // Connect to system clipboard's changed signal to track history
    QClipboard* sysClipboard = d->getSystemClipboard();
    if (sysClipboard) {
        connect(sysClipboard, &QClipboard::dataChanged,
                this, &Clipboard::onSystemClipboardChanged);
    }
}

Clipboard::~Clipboard()
{
    // Clear history to delete contained QMimeData objects
    d->history.clear();
}

void Clipboard::setText(const QString& text)
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    if (sysClipboard) {
        sysClipboard->setText(text);
        // Signal 'changed' is emitted by system clipboard, triggering onSystemClipboardChanged
    }
}

void Clipboard::setHtml(const QString& html)
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    if (sysClipboard) {
        QString sanitizedHtml = d->sanitizeHtml(html); // Sanitize before setting
        sysClipboard->setHtml(sanitizedHtml);
    }
}

void Clipboard::setImage(const QImage& image)
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    if (sysClipboard) {
        sysClipboard->setImage(image);
    }
}

void Clipboard::setData(QMimeData* data)
{
    if (!data) return;

    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    if (sysClipboard) {
        sysClipboard->setMimeData(data); // Clipboard takes ownership
    }
}

QString Clipboard::text() const
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    return sysClipboard ? sysClipboard->text() : QString();
}

QString Clipboard::html() const
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    return sysClipboard ? sysClipboard->html() : QString();
}

QImage Clipboard::image() const
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    return sysClipboard ? sysClipboard->image() : QImage();
}

const QMimeData* Clipboard::data() const
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    return sysClipboard ? sysClipboard->mimeData() : nullptr;
}

bool Clipboard::hasText() const
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    return sysClipboard ? sysClipboard->hasText() : false;
}

bool Clipboard::hasHtml() const
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    return sysClipboard ? sysClipboard->hasHtml() : false;
}

bool Clipboard::hasImage() const
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    return sysClipboard ? sysClipboard->hasImage() : false;
}

bool Clipboard::hasUrls() const
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    return sysClipboard ? sysClipboard->hasUrls() : false;
}

QList<QUrl> Clipboard::urls() const
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    return sysClipboard ? sysClipboard->urls() : QList<QUrl>();
}

void Clipboard::clear()
{
    QMutexLocker locker(&d->mutex);
    QClipboard* sysClipboard = d->getSystemClipboard();
    if (sysClipboard) {
        sysClipboard->clear();
    }
    emit cleared();
}

void Clipboard::setHistoryEnabled(bool enabled)
{
    QMutexLocker locker(&d->mutex);
    d->historyEnabled = enabled;
    if (!enabled) {
        clearHistory(); // Optionally clear history when disabling
    }
}

bool Clipboard::isHistoryEnabled() const
{
    QMutexLocker locker(&d->mutex);
    return d->historyEnabled;
}

QList<Clipboard::HistoryEntry> Clipboard::history() const
{
    QMutexLocker locker(&d->mutex);
    return d->history; // Returns a copy
}

int Clipboard::historySize() const
{
    QMutexLocker locker(&d->mutex);
    return d->history.size();
}

int Clipboard::maxHistorySize() const
{
    QMutexLocker locker(&d->mutex);
    return d->maxHistorySizeVal;
}

void Clipboard::setMaxHistorySize(int size)
{
    if (size < 0) return;
    QMutexLocker locker(&d->mutex);
    if (d->maxHistorySizeVal != size) {
        d->maxHistorySizeVal = size;
        // Trim history if new size is smaller
        while (d->history.size() > d->maxHistorySizeVal) {
            d->history.removeFirst();
        }
        LOG_DEBUG("Set clipboard history max size to " << size);
    }
}

bool Clipboard::restoreFromHistory(int index)
{
    QMutexLocker locker(&d->mutex);
    if (index < 0 || index >= d->history.size()) {
        LOG_WARN("Cannot restore from clipboard history: invalid index " << index);
        return false;
    }

    QClipboard* sysClipboard = d->getSystemClipboard();
    if (!sysClipboard) return false;

    const HistoryEntry& entry = d->history.at(index);
    if (!entry.data) {
        LOG_WARN("Cannot restore from clipboard history: null data at index " << index);
        return false;
    }

    // Clone the data from history to set on the system clipboard
    QMimeData* clonedData = d->cloneMimeData(entry.data);
    if (clonedData) {
        sysClipboard->setMimeData(clonedData); // System clipboard takes ownership
        LOG_DEBUG("Restored clipboard content from history index " << index);
        return true;
    }
    return false;
}

void Clipboard::clearHistory()
{
    QMutexLocker locker(&d->mutex);
    d->history.clear();
    emit historyChanged();
    LOG_DEBUG("Cleared clipboard history.");
}

QStringList Clipboard::formats() const
{
    QMutexLocker locker(&d->mutex);
    const QMimeData* data = this->data();
    return data ? data->formats() : QStringList();
}

bool Clipboard::hasFormat(const QString& mimeType) const
{
    QMutexLocker locker(&d->mutex);
    const QMimeData* data = this->data();
    return data ? data->hasFormat(mimeType) : false;
}

QByteArray Clipboard::rawData(const QString& mimeType) const
{
    QMutexLocker locker(&d->mutex);
    const QMimeData* data = this->data();
    return data ? data->data(mimeType) : QByteArray();
}

QMimeData* Clipboard::sanitizeData(QMimeData* data) const
{
    if (!data) return nullptr;

    QMimeData* sanitized = new QMimeData();
    // Copy safe formats, sanitize HTML
    if (data->hasText()) sanitized->setText(data->text());
    if (data->hasHtml()) sanitized->setHtml(d->sanitizeHtml(data->html()));
    if (data->hasImage()) sanitized->setImageData(data->imageData());
    if (data->hasUrls()) sanitized->setUrls(data->urls());
    if (data->hasColor()) sanitized->setColorData(data->colorData());

    // Copy other data as-is, or apply specific sanitization per format if needed
    for (const QString& format : data->formats()) {
        if (!sanitized->hasFormat(format)) {
            sanitized->setData(format, data->data(format));
        }
    }
    return sanitized;
}

void Clipboard::onSystemClipboardChanged()
{
    // This slot runs on the main thread where QApplication lives
    QMutexLocker locker(&d->mutex);
    d->addToHistoryIfNeeded();
    emit changed(); // Propagate the system change signal
}

} // namespace QuantilyxDoc