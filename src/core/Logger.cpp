/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "Logger.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QThread>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <iostream>

namespace QuantilyxDoc {

// Private implementation
class Logger::Private
{
public:
    Private() 
        : level(LogLevel::Info)
        , consoleOutput(false)
        , fileOutput(true)
        , timestamps(true)
        , threadIds(false)
        , functionNames(true)
        , maxFileSizeMB(10)
        , maxFiles(5)
        , logFile(nullptr)
    {}
    
    ~Private() {
        if (logFile) {
            logFile->close();
            delete logFile;
        }
    }

    LogLevel level;
    bool consoleOutput;
    bool fileOutput;
    bool timestamps;
    bool threadIds;
    bool functionNames;
    int maxFileSizeMB;
    int maxFiles;
    QString logFilePath;
    QFile* logFile;
};

Logger::Logger()
    : QObject(nullptr)
    , d(new Private())
{
}

Logger::~Logger()
{
    flush();
}

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::initialize(LogLevel level, const QString& logFile)
{
    QMutexLocker locker(&mutex);
    
    d->level = level;
    
    // Determine log file path
    if (logFile.isEmpty()) {
        QString logDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + 
                        "/quantilyxdoc/logs";
        QDir().mkpath(logDir);
        d->logFilePath = logDir + "/quantilyxdoc.log";
    } else {
        d->logFilePath = logFile;
    }
    
    // Open log file
    if (d->logFile) {
        d->logFile->close();
        delete d->logFile;
    }
    
    d->logFile = new QFile(d->logFilePath);
    if (!d->logFile->open(QIODevice::Append | QIODevice::Text)) {
        std::cerr << "Failed to open log file: " << d->logFilePath.toStdString() << std::endl;
        d->fileOutput = false;
    }
    
    // Log initialization
    log(LogLevel::Info, "=== Logger initialized ===");
    log(LogLevel::Info, QString("Log file: %1").arg(d->logFilePath));
    log(LogLevel::Info, QString("Log level: %1").arg(levelString(d->level)));
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&mutex);
    d->level = level;
}

LogLevel Logger::logLevel() const
{
    QMutexLocker locker(&const_cast<QMutex&>(mutex));
    return d->level;
}

void Logger::setConsoleOutput(bool enable)
{
    QMutexLocker locker(&mutex);
    d->consoleOutput = enable;
}

void Logger::setFileOutput(bool enable)
{
    QMutexLocker locker(&mutex);
    d->fileOutput = enable;
}

void Logger::setLogFilePath(const QString& filePath)
{
    QMutexLocker locker(&mutex);
    
    if (d->logFile) {
        d->logFile->close();
        delete d->logFile;
        d->logFile = nullptr;
    }
    
    d->logFilePath = filePath;
    
    d->logFile = new QFile(d->logFilePath);
    if (!d->logFile->open(QIODevice::Append | QIODevice::Text)) {
        std::cerr << "Failed to open log file: " << d->logFilePath.toStdString() << std::endl;
        d->fileOutput = false;
    }
}

QString Logger::logFilePath() const
{
    QMutexLocker locker(&const_cast<QMutex&>(mutex));
    return d->logFilePath;
}

void Logger::setTimestamps(bool enable)
{
    QMutexLocker locker(&mutex);
    d->timestamps = enable;
}

void Logger::setThreadIds(bool enable)
{
    QMutexLocker locker(&mutex);
    d->threadIds = enable;
}

void Logger::setFunctionNames(bool enable)
{
    QMutexLocker locker(&mutex);
    d->functionNames = enable;
}

void Logger::setMaxFileSize(int sizeMB)
{
    QMutexLocker locker(&mutex);
    d->maxFileSizeMB = sizeMB;
}

void Logger::setMaxFiles(int count)
{
    QMutexLocker locker(&mutex);
    d->maxFiles = count;
}

void Logger::log(LogLevel level, const QString& message, 
                const char* file, int line, const char* function)
{
    QMutexLocker locker(&mutex);
    
    // Check if message should be logged
    if (level < d->level) {
        return;
    }
    
    // Format message
    QString formattedMessage = formatMessage(level, message, file, line, function);
    
    // Output to console
    if (d->consoleOutput) {
        if (level >= LogLevel::Error) {
            std::cerr << formattedMessage.toStdString() << std::endl;
        } else {
            std::cout << formattedMessage.toStdString() << std::endl;
        }
    }
    
    // Output to file
    if (d->fileOutput && d->logFile) {
        checkRotation();
        writeToFile(formattedMessage);
    }
    
    // Emit signal
    emit messageLogged(level, formattedMessage);
}

void Logger::flush()
{
    QMutexLocker locker(&mutex);
    
    if (d->logFile) {
        d->logFile->flush();
    }
}

void Logger::clear()
{
    QMutexLocker locker(&mutex);
    
    if (d->logFile) {
        d->logFile->close();
        d->logFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    }
}

void Logger::rotate()
{
    QMutexLocker locker(&mutex);
    
    if (!d->logFile) {
        return;
    }
    
    d->logFile->close();
    
    // Rotate existing log files
    for (int i = d->maxFiles - 1; i >= 1; --i) {
        QString oldName = d->logFilePath + QString(".%1").arg(i);
        QString newName = d->logFilePath + QString(".%1").arg(i + 1);
        
        if (QFile::exists(newName)) {
            QFile::remove(newName);
        }
        
        if (QFile::exists(oldName)) {
            QFile::rename(oldName, newName);
        }
    }
    
    // Move current log to .1
    QString rotatedName = d->logFilePath + ".1";
    if (QFile::exists(rotatedName)) {
        QFile::remove(rotatedName);
    }
    QFile::rename(d->logFilePath, rotatedName);
    
    // Open new log file
    d->logFile->open(QIODevice::Append | QIODevice::Text);
    
    log(LogLevel::Info, "=== Log rotated ===");
}

QString Logger::formatMessage(LogLevel level, const QString& message,
                              const char* file, int line, const char* function) const
{
    QString formatted;
    
    // Timestamp
    if (d->timestamps) {
        formatted += QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz] ");
    }
    
    // Level
    formatted += QString("[%1] ").arg(levelString(level));
    
    // Thread ID
    if (d->threadIds) {
        formatted += QString("[Thread 0x%1] ")
                    .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16);
    }
    
    // Function name
    if (d->functionNames && function) {
        formatted += QString("[%1] ").arg(function);
    }
    
    // File and line
    if (file && line > 0) {
        QString filename = QFileInfo(file).fileName();
        formatted += QString("[%1:%2] ").arg(filename).arg(line);
    }
    
    // Message
    formatted += message;
    
    return formatted;
}

QString Logger::levelString(LogLevel level) const
{
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO ";
        case LogLevel::Warning:  return "WARN ";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT ";
        default:                 return "?????";
    }
}

void Logger::writeToFile(const QString& message)
{
    if (!d->logFile || !d->logFile->isOpen()) {
        return;
    }
    
    QTextStream stream(d->logFile);
    stream << message << Qt::endl;
    stream.flush();
}

void Logger::checkRotation()
{
    if (!d->logFile || !d->logFile->isOpen()) {
        return;
    }
    
    qint64 currentSize = d->logFile->size();
    qint64 maxSize = static_cast<qint64>(d->maxFileSizeMB) * 1024 * 1024;
    
    if (currentSize >= maxSize) {
        rotate();
    }
}

} // namespace QuantilyxDoc