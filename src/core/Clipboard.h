/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_CLIPBOARD_H
#define QUANTILYX_CLIPBOARD_H

#include <QObject>
#include <QMimeData>
#include <QImage>
#include <QByteArray>
#include <QUrl>
#include <memory>

namespace QuantilyxDoc {

/**
 * @brief Enhanced clipboard manager for document-specific data.
 *
 * Provides a higher-level interface for interacting with the system clipboard,
 * handling complex data types relevant to document editing (e.g., formatted text,
 * images, document fragments) and managing clipboard history.
 */
class Clipboard : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Represents an entry in the clipboard history.
     */
    struct HistoryEntry {
        QMimeData* data;                  // The clipboard data
        QDateTime timestamp;              // When it was added
        QString previewText;              // Short preview string
        QString dataType;                 // Primary data type (e.g., "text/plain", "image/png", "application/pdf")
        qint64 dataSize;                  // Approximate size in bytes

        HistoryEntry() : data(nullptr), dataSize(0) {}
        ~HistoryEntry() { delete data; } // Ensure data is cleaned up
    };

    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit Clipboard(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~Clipboard() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global Clipboard instance.
     */
    static Clipboard& instance();

    /**
     * @brief Set plain text data to the clipboard.
     * @param text The text to set.
     */
    void setText(const QString& text);

    /**
     * @brief Set rich text (HTML) data to the clipboard.
     * @param html The HTML string.
     */
    void setHtml(const QString& html);

    /**
     * @brief Set an image to the clipboard.
     * @param image The image to set.
     */
    void setImage(const QImage& image);

    /**
     * @brief Set custom MIME data to the clipboard.
     * @param data The MIME data object.
     */
    void setData(QMimeData* data);

    /**
     * @brief Get plain text from the clipboard.
     * @return The text content.
     */
    QString text() const;

    /**
     * @brief Get rich text (HTML) from the clipboard.
     * @return The HTML content.
     */
    QString html() const;

    /**
     * @brief Get an image from the clipboard.
     * @return The image, or a null image if none available.
     */
    QImage image() const;

    /**
     * @brief Get raw MIME data from the clipboard.
     * @return Pointer to the MIME data object (do not delete).
     */
    const QMimeData* data() const;

    /**
     * @brief Check if the clipboard contains plain text.
     * @return True if text is available.
     */
    bool hasText() const;

    /**
     * @brief Check if the clipboard contains HTML.
     * @return True if HTML is available.
     */
    bool hasHtml() const;

    /**
     * @brief Check if the clipboard contains an image.
     * @return True if an image is available.
     */
    bool hasImage() const;

    /**
     * @brief Check if the clipboard contains URLs.
     * @return True if URLs are available.
     */
    bool hasUrls() const;

    /**
     * @brief Get URLs from the clipboard.
     * @return List of URLs.
     */
    QList<QUrl> urls() const;

    /**
     * @brief Clear the clipboard contents.
     */
    void clear();

    /**
     * @brief Enable or disable clipboard history tracking.
     * @param enabled True to enable history.
     */
    void setHistoryEnabled(bool enabled);

    /**
     * @brief Check if clipboard history is enabled.
     * @return True if enabled.
     */
    bool isHistoryEnabled() const;

    /**
     * @brief Get the clipboard history.
     * @return List of history entries (oldest first).
     */
    QList<HistoryEntry> history() const;

    /**
     * @brief Get the number of items in the clipboard history.
     * @return History size.
     */
    int historySize() const;

    /**
     * @brief Get the maximum number of items to keep in history.
     * @return Maximum history count.
     */
    int maxHistorySize() const;

    /**
     * @brief Set the maximum number of items to keep in history.
     * @param size New maximum size.
     */
    void setMaxHistorySize(int size);

    /**
     * @brief Restore a specific item from history to the clipboard.
     * @param index Index of the item in the history list.
     * @return True if restoration was successful.
     */
    bool restoreFromHistory(int index);

    /**
     * @brief Clear the clipboard history.
     */
    void clearHistory();

    /**
     * @brief Get the MIME types currently available on the clipboard.
     * @return List of MIME type strings.
     */
    QStringList formats() const;

    /**
     * @brief Check if the clipboard contains a specific MIME type.
     * @param mimeType The MIME type string.
     * @return True if the type is available.
     */
    bool hasFormat(const QString& mimeType) const;

    /**
     * @brief Get raw data for a specific MIME type.
     * @param mimeType The MIME type string.
     * @return Raw byte array.
     */
    QByteArray rawData(const QString& mimeType) const;

    /**
     * @brief Sanitize clipboard data before setting it.
     * Removes potentially dangerous content like scripts from HTML.
     * @param data The data to sanitize.
     * @return Sanitized data object.
     */
    QMimeData* sanitizeData(QMimeData* data) const;

signals:
    /**
     * @brief Emitted when clipboard content changes.
     */
    void changed();

    /**
     * @brief Emitted when clipboard history is updated.
     */
    void historyChanged();

    /**
     * @brief Emitted when an item is added to the history.
     * @param entry The new history entry.
     */
    void historyItemAdded(const QuantilyxDoc::Clipboard::HistoryEntry& entry);

    /**
     * @brief Emitted when the clipboard is cleared.
     */
    void cleared();

private slots:
    void onSystemClipboardChanged();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_CLIPBOARD_H