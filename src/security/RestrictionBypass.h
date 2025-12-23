/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_RESTRICTIONBYPASS_H
#define QUANTILYX_RESTRICTIONBYPASS_H

#include <QObject>
#include <QMutex>
#include <memory>

namespace QuantilyxDoc {

class Document; // Forward declaration

/**
 * @brief Bypasses document restrictions (e.g., printing, copying, editing).
 * 
 * Uses external tools or libraries (e.g., QPDF for PDFs) to remove permissions
 * and restrictions imposed by the document creator, liberating the document.
 * This aligns with the QuantilyxDoc philosophy of document liberation.
 */
class RestrictionBypass : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit RestrictionBypass(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~RestrictionBypass() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global RestrictionBypass instance.
     */
    static RestrictionBypass& instance();

    /**
     * @brief Bypass restrictions from a document file.
     * @param inputFilePath Path to the restricted input file.
     * @param outputFilePath Path to save the unrestricted output file.
     * @return True if the restriction bypass was successful.
     */
    bool bypassRestrictions(const QString& inputFilePath, const QString& outputFilePath);

    /**
     * @brief Bypass restrictions from an already loaded Document object.
     * This might involve saving the document to a temporary file, bypassing restrictions, and reloading.
     * @param document The loaded document object (must have restrictions).
     * @return True if the restriction bypass was successful and the document is reloaded unrestricted.
     */
    bool bypassRestrictionsFromDocument(Document* document);

    /**
     * @brief Check if a specific file format is supported for restriction bypass.
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
     * @brief Get the path to the external tool used for bypass (e.g., qpdf executable).
     * @return Tool path string.
     */
    QString externalToolPath() const;

    /**
     * @brief Set the path to the external tool used for bypass.
     * @param path Tool path string.
     */
    void setExternalToolPath(const QString& path);

    /**
     * @brief Get a list of restrictions detected in a document file.
     * @param filePath Path to the file to analyze.
     * @return List of restriction types (e.g., "Printing", "Copying", "Editing").
     */
    QStringList detectRestrictions(const QString& filePath) const;

signals:
    /**
     * @brief Emitted when restriction bypass starts.
     * @param inputPath Path to the input file.
     */
    void bypassStarted(const QString& inputPath);

    /**
     * @brief Emitted when restriction bypass finishes successfully.
     * @param inputPath Path to the input file.
     * @param outputPath Path to the output file.
     */
    void bypassFinished(const QString& inputPath, const QString& outputPath);

    /**
     * @brief Emitted when restriction bypass fails.
     * @param inputPath Path to the input file.
     * @param error Error message.
     */
    void bypassFailed(const QString& inputPath, const QString& error);

    /**
     * @brief Emitted periodically during a long-running bypass task.
     * @param progress Progress percentage (0-100).
     */
    void bypassProgress(int progress);

private:
    class Private;
    std::unique_ptr<Private> d;

    // Helper to find the external tool executable (e.g., qpdf)
    QString findExternalTool() const;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_RESTRICTIONBYPASS_H