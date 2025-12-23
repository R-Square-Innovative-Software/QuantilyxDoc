/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "RestrictionBypass.h"
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

class RestrictionBypass::Private {
public:
    Private(RestrictionBypass* q_ptr)
        : q(q_ptr) {}

    RestrictionBypass* q;
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
                LOG_DEBUG("RestrictionBypass: Found QPDF at: " << fullPath);
                return fullPath;
            }
        }

    #ifdef Q_OS_WIN
        QString defaultPath = "C:/Program Files/qpdf/bin/qpdf.exe";
        if (QFile::exists(defaultPath)) {
            LOG_DEBUG("RestrictionBypass: Found QPDF at default Windows path: " << defaultPath);
            return defaultPath;
        }
    #endif

        LOG_ERROR("RestrictionBypass: QPDF executable not found. Please install QPDF.");
        return QString(); // Not found
    }

    // Helper to detect restrictions using QPDF (example command: qpdf --show-encryption input.pdf)
    QStringList detectRestrictionsWithQpdf(const QString& inputFilePath) const {
        QString qpdfPath = findQpdfExecutable();
        if (qpdfPath.isEmpty()) return QStringList();

        QStringList args;
        args << "--show-encryption" << inputFilePath;

        QProcess qpdfProcess;
        qpdfProcess.start(qpdfPath, args);
        bool finished = qpdfProcess.waitForFinished(-1);

        if (!finished) {
            LOG_ERROR("RestrictionBypass::detectRestrictionsWithQpdf: QPDF process did not finish.");
            return QStringList();
        }

        if (qpdfProcess.exitCode() != 0) {
            QString errorOutput = qpdfProcess.readAllStandardError();
            LOG_ERROR("RestrictionBypass::detectRestrictionsWithQpdf: QPDF failed: " << errorOutput);
            return QStringList();
        }

        QString output = qpdfProcess.readAllStandardOutput();
        LOG_DEBUG("RestrictionBypass::detectRestrictionsWithQpdf: QPDF output: " << output);

        // Parse the output to find restriction flags
        QStringList restrictions;
        if (output.contains("allow-print", Qt::CaseInsensitive)) {
            // Check if 'print' is *disallowed*
            if (!output.contains("allow-print: true", Qt::CaseInsensitive)) {
                restrictions.append("Printing");
            }
        }
        if (output.contains("allow-plaintext-metadata", Qt::CaseInsensitive)) {
            // Check if 'copy/extract' is *disallowed*
            if (!output.contains("allow-plaintext-meta true", Qt::CaseInsensitive)) {
                restrictions.append("Copying");
            }
        }
        // Add checks for other permissions (editing, annotating, etc.) as needed based on QPDF output format.
        // The exact strings depend on QPDF's output format.

        return restrictions;
    }
};

// Static instance pointer
RestrictionBypass* RestrictionBypass::s_instance = nullptr;

RestrictionBypass& RestrictionBypass::instance()
{
    if (!s_instance) {
        s_instance = new RestrictionBypass();
    }
    return *s_instance;
}

RestrictionBypass::RestrictionBypass(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    LOG_INFO("RestrictionBypass created.");
}

RestrictionBypass::~RestrictionBypass()
{
    LOG_INFO("RestrictionBypass destroyed.");
}

bool RestrictionBypass::bypassRestrictions(const QString& inputFilePath, const QString& outputFilePath)
{
    QString qpdfPath = d->findQpdfExecutable();
    if (qpdfPath.isEmpty()) {
        LOG_ERROR("RestrictionBypass::bypassRestrictions: QPDF executable not found.");
        emit bypassFailed(inputFilePath, "QPDF tool not found. Please install QPDF.");
        return false;
    }

    emit bypassStarted(inputFilePath);

    // Build QPDF command arguments to remove restrictions
    // Example: qpdf --decrypt input.pdf output.pdf
    // Note: --decrypt removes encryption AND restrictions if it has the owner password (often the master key).
    // If only user password is known, it might not remove all restrictions.
    // QPDF also has --set-encrypt flags to *set* permissions to allow everything.
    QStringList args;
    args << "--decrypt"; // This is the primary command for removing restrictions via owner password
    // Alternative: args << "--set-encrypt=none"; // Explicitly set no encryption/permissions (requires owner password to read original)
    // Another alternative: args << "--stream-data=uncompress" << "--object-streams=disable"; // Sometimes helps with weird restrictions
    args << inputFilePath << outputFilePath;

    LOG_DEBUG("RestrictionBypass::bypassRestrictions: Executing: " << qpdfPath << " " << args.join(" "));

    // Run QPDF process
    QProcess qpdfProcess;
    qpdfProcess.start(qpdfPath, args);
    bool finished = qpdfProcess.waitForFinished(-1);

    if (!finished) {
        QString error = "QPDF process did not finish.";
        LOG_ERROR("RestrictionBypass::bypassRestrictions: " << error);
        emit bypassFailed(inputFilePath, error);
        return false;
    }

    if (qpdfProcess.exitCode() != 0) {
        QString errorOutput = qpdfProcess.readAllStandardError();
        LOG_ERROR("RestrictionBypass::bypassRestrictions: QPDF failed with exit code " << qpdfProcess.exitCode() << ". Error: " << errorOutput);
        emit bypassFailed(inputFilePath, errorOutput);
        return false;
    }

    // Check if output file was created
    if (!QFile::exists(outputFilePath)) {
        QString error = "QPDF did not create output file: " + outputFilePath;
        LOG_ERROR("RestrictionBypass::bypassRestrictions: " << error);
        emit bypassFailed(inputFilePath, error);
        return false;
    }

    LOG_INFO("RestrictionBypass::bypassRestrictions: Successfully bypassed restrictions, saved to: " << outputFilePath);
    emit bypassFinished(inputFilePath, outputFilePath);
    return true;
}

bool RestrictionBypass::bypassRestrictionsFromDocument(Document* document)
{
    if (!document) {
        LOG_ERROR("RestrictionBypass::bypassRestrictionsFromDocument: Null document provided.");
        return false;
    }

    QString inputPath = document->filePath();
    if (inputPath.isEmpty()) {
        LOG_ERROR("RestrictionBypass::bypassRestrictionsFromDocument: Document has no file path.");
        return false;
    }

    // Create a temporary file for the unrestricted version
    QTemporaryFile tempOutput;
    tempOutput.setFileTemplate(QDir::tempPath() + "/quantilyx_unrestricted_XXXXXX." + QFileInfo(inputPath).suffix());
    if (!tempOutput.open()) {
        LOG_ERROR("RestrictionBypass::bypassRestrictionsFromDocument: Failed to create temporary output file.");
        return false;
    }
    QString outputPath = tempOutput.fileName();
    tempOutput.close(); // Close so QPDF can write to it

    // Perform the bypass
    bool success = bypassRestrictions(inputPath, outputPath);
    if (success) {
        LOG_INFO("RestrictionBypass::bypassRestrictionsFromDocument: Successfully bypassed restrictions. Output saved to: " << outputPath);
        // The application might want to prompt the user to save the unrestricted version permanently.
    } else {
        LOG_ERROR("RestrictionBypass::bypassRestrictionsFromDocument: Failed to bypass restrictions from document: " << inputPath);
        QFile::remove(outputPath); // Clean up temporary file if bypass failed
    }

    return success;
}

bool RestrictionBypass::isFormatSupported(const QString& filePath) const
{
    QFileInfo info(filePath);
    QString suffix = info.suffix().toLower();
    return supportedFormats().contains(suffix);
}

QStringList RestrictionBypass::supportedFormats() const
{
    // Like PasswordRemover, primarily supports PDF via QPDF.
    return QStringList() << "pdf";
}

QString RestrictionBypass::externalToolPath() const
{
    QMutexLocker locker(&d->mutex);
    return d->externalToolPathStr;
}

void RestrictionBypass::setExternalToolPath(const QString& path)
{
    QMutexLocker locker(&d->mutex);
    if (d->externalToolPathStr != path) {
        d->externalToolPathStr = path;
        LOG_INFO("RestrictionBypass: External tool path set to: " << path);
    }
}

QStringList RestrictionBypass::detectRestrictions(const QString& filePath) const
{
    // Use QPDF to analyze the file and list permissions.
    return d->detectRestrictionsWithQpdf(filePath);
}

QString RestrictionBypass::findExternalTool() const
{
    return d->findQpdfExecutable();
}

} // namespace QuantilyxDoc