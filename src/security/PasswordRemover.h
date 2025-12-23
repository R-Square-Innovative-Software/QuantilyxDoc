/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_PASSWORDREMOVER_H
#define QUANTILYX_PASSWORDREMOVER_H

#include <QObject>
#include <QMutex>
#include <memory>

namespace QuantilyxDoc {

class Document; // Forward declaration

/**
 * @brief Removes passwords from documents that support it.
 * 
 * Uses external tools or libraries (e.g., QPDF for PDFs) to strip passwords
 * and access restrictions, liberating the document for unrestricted use.
 * This aligns with the QuantilyxDoc philosophy of document liberation.
 */
class PasswordRemover : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit PasswordRemover(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~PasswordRemover() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global PasswordRemover instance.
     */
    static PasswordRemover& instance();

    /**
     * @brief Remove the password from a document file.
     * @param inputFilePath Path to the password-protected input file.
     * @param outputFilePath Path to save the unlocked output file.
     * @param userPassword The user password (if required for opening, but not owner password).
     * @return True if the password removal was successful.
     */
    bool removePassword(const QString& inputFilePath, const QString& outputFilePath, const QString& userPassword = QString());

    /**
     * @brief Remove the password from an already loaded Document object.
     * This might involve saving the document to a temporary file, unlocking it, and reloading.
     * @param document The loaded document object (must be password-protected).
     * @param userPassword The user password.
     * @return True if the password removal was successful and the document is reloaded unlocked.
     */
    bool removePasswordFromDocument(Document* document, const QString& userPassword = QString());

    /**
     * @brief Check if a specific file format is supported for password removal.
     * @param filePath Path to the file to check.
     * @return True if the format is supported.
     */
    bool isFormatSupported(const QString& filePath) const;

    /**
     * @brief Get the list of supported file formats (extensions).
     * @return List of supported extensions (e.g., "pdf").
     */
    QStringList supportedFormats() const;

    /**
     * @brief Get the path to the external tool used for removal (e.g., qpdf executable).
     * @return Tool path string.
     */
    QString externalToolPath() const;

    /**
     * @brief Set the path to the external tool used for removal.
     * @param path Tool path string.
     */
    void setExternalToolPath(const QString& path);

signals:
    /**
     * @brief Emitted when password removal starts.
     * @param inputPath Path to the input file.
     */
    void removalStarted(const QString& inputPath);

    /**
     * @brief Emitted when password removal finishes successfully.
     * @param inputPath Path to the input file.
     * @param outputPath Path to the output file.
     */
    void removalFinished(const QString& inputPath, const QString& outputPath);

    /**
     * @brief Emitted when password removal fails.
     * @param inputPath Path to the input file.
     * @param error Error message.
     */
    void removalFailed(const QString& inputPath, const QString& error);

    /**
     * @brief Emitted periodically during a long-running removal task.
     * @param progress Progress percentage (0-100).
     */
    void removalProgress(int progress);

private:
    class Private;
    std::unique_ptr<Private> d;

    // Helper to find the external tool executable (e.g., qpdf)
    QString findExternalTool() const;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_PASSWORDREMOVER_H