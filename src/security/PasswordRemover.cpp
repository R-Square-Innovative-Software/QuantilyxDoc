/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "PasswordRemover.h"
#include "../core/Document.h"
#include "../core/Logger.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QMutex>
#include <QMutexLocker>
#include <QTemporaryFile>
#include <QDebug>

namespace QuantilyxDoc {

class PasswordRemover::Private {
public:
    Private(PasswordRemover* q_ptr)
        : q(q_ptr) {}

    PasswordRemover* q;
    mutable QMutex mutex; // Protect access if needed during process execution
    QString externalToolPathStr;

    // Helper to find the QPDF executable
    QString findQpdfExecutable() const {
        QStringList possibleNames = {
    #ifdef Q_OS_WIN
            "qpdf.exe"
    #else
            "qpdf"
    #endif
        };

        for (const QString& name : possibleNames) {
            QString fullPath = QStandardPaths::findExecutable(name);
            if (!fullPath.isEmpty()) {
                LOG_DEBUG("PasswordRemover: Found QPDF at: " << fullPath);
                return fullPath;
            }
        }

        // If not found in PATH, check common installation directories (platform-specific)
        // This is complex and might require user configuration.
        // Example for Windows default install:
    #ifdef Q_OS_WIN
        QString defaultPath = "C:/Program Files/qpdf/bin/qpdf.exe";
        if (QFile::exists(defaultPath)) {
            LOG_DEBUG("PasswordRemover: Found QPDF at default Windows path: " << defaultPath);
            return defaultPath;
        }
    #endif

        LOG_ERROR("PasswordRemover: QPDF executable not found. Please install QPDF.");
        return QString(); // Not found
    }
};

// Static instance pointer
PasswordRemover* PasswordRemover::s_instance = nullptr;

PasswordRemover& PasswordRemover::instance()
{
    if (!s_instance) {
        s_instance = new PasswordRemover();
    }
    return *s_instance;
}

PasswordRemover::PasswordRemover(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    LOG_INFO("PasswordRemover created.");
}

PasswordRemover::~PasswordRemover()
{
    LOG_INFO("PasswordRemover destroyed.");
}

bool PasswordRemover::removePassword(const QString& inputFilePath, const QString& outputFilePath, const QString& userPassword)
{
    QString qpdfPath = d->findQpdfExecutable();
    if (qpdfPath.isEmpty()) {
        LOG_ERROR("PasswordRemover::removePassword: QPDF executable not found.");
        emit removalFailed(inputFilePath, "QPDF tool not found. Please install QPDF.");
        return false;
    }

    emit removalStarted(inputFilePath);

    // Build QPDF command arguments
    // Example: qpdf --password=<user_password> --remove-password input.pdf output.pdf
    QStringList args;
    if (!userPassword.isEmpty()) {
        args << "--password=" + userPassword;
    } else {
        // If no password provided, qpdf might try to open without one (if it's an open-password doc with only owner pwd)
        // Or it might fail. Let's assume owner password removal works without user password if possible.
        // QPDF might have other flags for specific scenarios.
        LOG_WARN("PasswordRemover::removePassword: No password provided. Attempting removal without password (may fail if file is open-password protected).");
    }
    args << "--remove-password" << inputFilePath << outputFilePath;

    LOG_DEBUG("PasswordRemover::removePassword: Executing: " << qpdfPath << " " << args.join(" "));

    // Run QPDF process
    QProcess qpdfProcess;
    qpdfProcess.start(qpdfPath, args);
    bool finished = qpdfProcess.waitForFinished(-1); // Wait indefinitely, or set a timeout

    if (!finished) {
        QString error = "QPDF process did not finish.";
        LOG_ERROR("PasswordRemover::removePassword: " << error);
        emit removalFailed(inputFilePath, error);
        return false;
    }

    if (qpdfProcess.exitCode() != 0) {
        QString errorOutput = qpdfProcess.readAllStandardError();
        LOG_ERROR("PasswordRemover::removePassword: QPDF failed with exit code " << qpdfProcess.exitCode() << ". Error: " << errorOutput);
        emit removalFailed(inputFilePath, errorOutput);
        return false;
    }

    // Check if output file was created
    if (!QFile::exists(outputFilePath)) {
        QString error = "QPDF did not create output file: " + outputFilePath;
        LOG_ERROR("PasswordRemover::removePassword: " << error);
        emit removalFailed(inputFilePath, error);
        return false;
    }

    LOG_INFO("PasswordRemover::removePassword: Successfully removed password, saved to: " << outputFilePath);
    emit removalFinished(inputFilePath, outputFilePath);
    return true;
}

bool PasswordRemover::removePasswordFromDocument(Document* document, const QString& userPassword)
{
    if (!document) {
        LOG_ERROR("PasswordRemover::removePasswordFromDocument: Null document provided.");
        return false;
    }

    QString inputPath = document->filePath();
    if (inputPath.isEmpty()) {
        LOG_ERROR("PasswordRemover::removePasswordFromDocument: Document has no file path.");
        return false;
    }

    // Create a temporary file for the unlocked version
    QTemporaryFile tempOutput;
    tempOutput.setFileTemplate(QDir::tempPath() + "/quantilyx_unlocked_XXXXXX." + QFileInfo(inputPath).suffix());
    if (!tempOutput.open()) {
        LOG_ERROR("PasswordRemover::removePasswordFromDocument: Failed to create temporary output file.");
        return false;
    }
    QString outputPath = tempOutput.fileName();
    tempOutput.close(); // Close so QPDF can write to it

    // Perform the removal
    bool success = removePassword(inputPath, outputPath, userPassword);
    if (success) {
        // Attempt to reload the document from the unlocked temporary file
        // This requires the Document class to have a reload or loadFromPath method.
        // document->load(outputPath); // Hypothetical method
        LOG_INFO("PasswordRemover::removePasswordFromDocument: Successfully unlocked document. Output saved to: " << outputPath);
        // The application might want to prompt the user to save the unlocked version permanently.
    } else {
        LOG_ERROR("PasswordRemover::removePasswordFromDocument: Failed to remove password from document: " << inputPath);
        QFile::remove(outputPath); // Clean up temporary file if removal failed
    }

    return success;
}

bool PasswordRemover::isFormatSupported(const QString& filePath) const
{
    // Check file extension
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();
    return supportedFormats().contains(suffix);
}

QStringList PasswordRemover::supportedFormats() const
{
    // QPDF primarily supports PDF.
    // Other formats might require different tools (e.g., mutool for XPS/EPUB?, specific CAD tools?)
    // For now, focus on PDF.
    return QStringList() << "pdf";
}

QString PasswordRemover::externalToolPath() const
{
    QMutexLocker locker(&d->mutex);
    return d->externalToolPathStr;
}

void PasswordRemover::setExternalToolPath(const QString& path)
{
    QMutexLocker locker(&d->mutex);
    if (d->externalToolPathStr != path) {
        d->externalToolPathStr = path;
        LOG_INFO("PasswordRemover: External tool path set to: " << path);
    }
}

QString PasswordRemover::findExternalTool() const
{
    return d->findQpdfExecutable(); // Defer to private helper
}

} // namespace QuantilyxDoc