/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef QUANTILYX_LOGGER_H
#define QUANTILYX_LOGGER_H

#include <QString>
#include <QObject>
#include <QMutex>
#include <memory>
#include <sstream>

namespace QuantilyxDoc {

/**
 * @brief Log levels
 */
enum class LogLevel
{
    Debug,      ///< Debug messages
    Info,       ///< Informational messages
    Warning,    ///< Warning messages
    Error,      ///< Error messages
    Critical    ///< Critical error messages
};

/**
 * @brief Logger class - Singleton pattern
 * 
 * Thread-safe logging system with file rotation and multiple outputs.
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     * @return Reference to Logger
     */
    static Logger& instance();

    /**
     * @brief Destructor
     */
    ~Logger();

    /**
     * @brief Initialize logger
     * @param level Minimum log level
     * @param logFile Log file path (empty for default)
     */
    void initialize(LogLevel level = LogLevel::Info, const QString& logFile = QString());

    /**
     * @brief Set log level
     * @param level Minimum log level
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief Get current log level
     * @return Current log level
     */
    LogLevel logLevel() const;

    /**
     * @brief Enable/disable console output
     * @param enable true to enable console output
     */
    void setConsoleOutput(bool enable);

    /**
     * @brief Enable/disable file output
     * @param enable true to enable file output
     */
    void setFileOutput(bool enable);

    /**
     * @brief Set log file path
     * @param filePath Path to log file
     */
    void setLogFilePath(const QString& filePath);

    /**
     * @brief Get log file path
     * @return Log file path
     */
    QString logFilePath() const;

    /**
     * @brief Enable/disable timestamps
     * @param enable true to enable timestamps
     */
    void setTimestamps(bool enable);

    /**
     * @brief Enable/disable thread IDs
     * @param enable true to enable thread IDs
     */
    void setThreadIds(bool enable);

    /**
     * @brief Enable/disable function names
     * @param enable true to enable function names
     */
    void setFunctionNames(bool enable);

    /**
     * @brief Set maximum log file size (MB)
     * @param sizeMB Maximum size in megabytes
     */
    void setMaxFileSize(int sizeMB);

    /**
     * @brief Set maximum number of log files
     * @param count Maximum number of rotated log files
     */
    void setMaxFiles(int count);

    /**
     * @brief Log a message
     * @param level Log level
     * @param message Message to log
     * @param file Source file name
     * @param line Line number
     * @param function Function name
     */
    void log(LogLevel level, const QString& message, 
            const char* file = nullptr, int line = 0, const char* function = nullptr);

    /**
     * @brief Flush log buffers
     */
    void flush();

    /**
     * @brief Clear log file
     */
    void clear();

    /**
     * @brief Rotate log files
     */
    void rotate();

signals:
    /**
     * @brief Emitted when a message is logged
     * @param level Log level
     * @param message Log message
     */
    void messageLogged(LogLevel level, const QString& message);

private:
    /**
     * @brief Private constructor (Singleton)
     */
    Logger();

    /**
     * @brief Disable copy constructor
     */
    Logger(const Logger&) = delete;

    /**
     * @brief Disable assignment operator
     */
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Format log message
     * @param level Log level
     * @param message Message
     * @param file Source file
     * @param line Line number
     * @param function Function name
     * @return Formatted message
     */
    QString formatMessage(LogLevel level, const QString& message,
                         const char* file, int line, const char* function) const;

    /**
     * @brief Get level string
     * @param level Log level
     * @return Level string
     */
    QString levelString(LogLevel level) const;

    /**
     * @brief Write to file
     * @param message Message to write
     */
    void writeToFile(const QString& message);

    /**
     * @brief Check if log file needs rotation
     */
    void checkRotation();

private:
    class Private;
    std::unique_ptr<Private> d;
    QMutex mutex;
};

/**
 * @brief Log stream helper class
 */
class LogStream
{
public:
    LogStream(LogLevel level, const char* file, int line, const char* function)
        : m_level(level), m_file(file), m_line(line), m_function(function) {}
    
    ~LogStream() {
        Logger::instance().log(m_level, QString::fromStdString(m_stream.str()), 
                              m_file, m_line, m_function);
    }
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        m_stream << value;
        return *this;
    }
    
    LogStream& operator<<(const QString& value) {
        m_stream << value.toStdString();
        return *this;
    }
    
    LogStream& operator<<(const QByteArray& value) {
        m_stream << value.constData();
        return *this;
    }

private:
    LogLevel m_level;
    const char* m_file;
    int m_line;
    const char* m_function;
    std::ostringstream m_stream;
};

} // namespace QuantilyxDoc

// Convenient logging macros
#define LOG_DEBUG   QuantilyxDoc::LogStream(QuantilyxDoc::LogLevel::Debug, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO    QuantilyxDoc::LogStream(QuantilyxDoc::LogLevel::Info, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING QuantilyxDoc::LogStream(QuantilyxDoc::LogLevel::Warning, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR   QuantilyxDoc::LogStream(QuantilyxDoc::LogLevel::Error, __FILE__, __LINE__, __FUNCTION__)
#define LOG_CRITICAL QuantilyxDoc::LogStream(QuantilyxDoc::LogLevel::Critical, __FILE__, __LINE__, __FUNCTION__)

#endif // QUANTILYX_LOGGER_H