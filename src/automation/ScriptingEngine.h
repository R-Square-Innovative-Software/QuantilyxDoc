/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_SCRIPTINGENGINE_H
#define QUANTILYX_SCRIPTINGENGINE_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QMutex>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

class Document; // Forward declaration

/**
 * @brief Represents a loaded script.
 */
struct Script {
    QString id;                 // Unique identifier for the script
    QString name;               // Display name
    QString description;        // Description of what the script does
    QString content;            // The script code itself
    QString language;           // Language identifier (e.g., "python", "javascript")
    QString author;             // Author name
    QString version;            // Version string
    QDateTime lastModified;     // Timestamp of last edit
    bool isRunnable;            // Whether the script can be executed
    bool isAutostart;          // Whether the script should run on startup
    QStringList dependencies;   // List of other scripts or modules this script depends on
};

/**
 * @brief Integrates a scripting language (e.g., Python, JavaScript) into the application.
 * 
 * Allows users to write and execute scripts to automate tasks, extend functionality,
 * or interact with documents programmatically.
 */
class ScriptingEngine : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit ScriptingEngine(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ScriptingEngine() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global ScriptingEngine instance.
     */
    static ScriptingEngine& instance();

    /**
     * @brief Initialize the scripting engine.
     * Loads the underlying scripting library (e.g., Python interpreter, V8 engine).
     * @param language The scripting language to initialize (e.g., "python", "javascript").
     * @param interpreterPath Optional path to the interpreter executable (if needed).
     * @return True if initialization was successful.
     */
    bool initialize(const QString& language, const QString& interpreterPath = QString());

    /**
     * @brief Check if the scripting engine is initialized and ready.
     * @return True if ready.
     */
    bool isReady() const;

    /**
     * @brief Execute a script string directly.
     * @param scriptCode The script code to execute.
     * @param scriptName Optional name for the script (for error reporting).
     * @return True if execution was successful.
     */
    bool executeScript(const QString& scriptCode, const QString& scriptName = "inline_script");

    /**
     * @brief Execute a script file.
     * @param filePath Path to the script file.
     * @return True if execution was successful.
     */
    bool executeScriptFile(const QString& filePath);

    /**
     * @brief Evaluate a script expression and return the result.
     * @param expression The expression to evaluate.
     * @return The result of the evaluation as a QVariant.
     */
    QVariant evaluateExpression(const QString& expression);

    /**
     * @brief Load a script from a file into the engine's registry.
     * Does not execute it immediately.
     * @param filePath Path to the script file.
     * @return The loaded Script structure, or an invalid one if loading failed.
     */
    Script loadScript(const QString& filePath);

    /**
     * @brief Unload a script from the engine's registry.
     * @param scriptId The ID of the script to unload.
     * @return True if unloading was successful.
     */
    bool unloadScript(const QString& scriptId);

    /**
     * @brief Get the list of currently loaded scripts.
     * @return List of Script structures.
     */
    QList<Script> loadedScripts() const;

    /**
     * @brief Get a specific script by its ID.
     * @param scriptId The ID of the script.
     * @return The Script structure, or an invalid one if not found.
     */
    Script getScriptById(const QString& scriptId) const;

    /**
     * @brief Call a specific function within a loaded script.
     * @param scriptId The ID of the script containing the function.
     * @param functionName The name of the function to call.
     * @param args Arguments to pass to the function.
     * @return The result of the function call as a QVariant.
     */
    QVariant callFunction(const QString& scriptId, const QString& functionName, const QVariantList& args = QVariantList());

    /**
     * @brief Register a C++ function/object to be accessible from scripts.
     * This allows scripts to call core application functionality.
     * @param name The name under which the function/object will be available in scripts.
     * @param callable The C++ function or functor to register.
     */
    void registerCallable(const QString& name, const std::function<QVariant(const QVariantList&)>& callable);

    /**
     * @brief Register a C++ object to be accessible from scripts.
     * @param name The name under which the object will be available in scripts.
     * @param obj The C++ object to register.
     */
    void registerObject(const QString& name, QObject* obj);

    /**
     * @brief Get the currently active scripting language.
     * @return Language code string.
     */
    QString currentLanguage() const;

    /**
     * @brief Get the path to the scripting interpreter.
     * @return Interpreter path string.
     */
    QString interpreterPath() const;

    /**
     * @brief Get the list of supported scripting languages.
     * @return List of language code strings (e.g., "python", "javascript").
     */
    QStringList supportedLanguages() const;

    /**
     * @brief Interrupt the currently executing script (if possible).
     */
    void interruptExecution();

    /**
     * @brief Get the current execution state.
     * @return Execution state (e.g., Idle, Running, Interrupted).
     */
    QString executionState() const;

signals:
    /**
     * @brief Emitted when the scripting engine is initialized.
     * @param success Whether initialization was successful.
     */
    void initializationComplete(bool success);

    /**
     * @brief Emitted when a script starts executing.
     * @param scriptId The ID of the script.
     */
    void scriptStarted(const QString& scriptId);

    /**
     * @brief Emitted when a script finishes executing.
     * @param scriptId The ID of the script.
     * @param success Whether execution was successful.
     */
    void scriptFinished(const QString& scriptId, bool success);

    /**
     * @brief Emitted when a script execution fails.
     * @param scriptId The ID of the script.
     * @param error Error message.
     */
    void scriptFailed(const QString& scriptId, const QString& error);

    /**
     * @brief Emitted when a script is loaded.
     * @param script The loaded script structure.
     */
    void scriptLoaded(const QuantilyxDoc::Script& script);

    /**
     * @brief Emitted when a script is unloaded.
     * @param scriptId The ID of the unloaded script.
     */
    void scriptUnloaded(const QString& scriptId);

    /**
     * @brief Emitted when a script prints output.
     * @param output The printed text.
     */
    void scriptOutput(const QString& output);

    /**
     * @brief Emitted when a script logs a message.
     * @param level Log level (e.g., "INFO", "WARN", "ERROR").
     * @param message Log message.
     */
    void scriptLog(const QString& level, const QString& message);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_SCRIPTINGENGINE_H