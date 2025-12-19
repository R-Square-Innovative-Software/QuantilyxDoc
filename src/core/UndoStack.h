/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_UNDOSTACK_H
#define QUANTILYX_UNDOSTACK_H

#include <QObject>
#include <QList>
#include <QUndoCommand>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

class Document;

/**
 * @brief Extended undo stack supporting unlimited levels and branching.
 * 
 * Extends Qt's QUndoStack to potentially add features like:
 * - Visual undo/redo tree visualization (requires UndoVisualization).
 * - Better integration with document states.
 * - Custom command grouping.
 * For now, it acts as a wrapper around QUndoStack.
 */
class UndoStack : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit UndoStack(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~UndoStack() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global UndoStack instance.
     */
    static UndoStack& instance();

    /**
     * @brief Push a command onto the stack.
     * @param cmd The command to push.
     */
    void push(QUndoCommand* cmd);

    /**
     * @brief Undo the last command.
     */
    void undo();

    /**
     * @brief Redo the last undone command.
     */
    void redo();

    /**
     * @brief Clear the undo stack.
     */
    void clear();

    /**
     * @brief Check if undo is available.
     * @return True if an undo operation is possible.
     */
    bool canUndo() const;

    /**
     * @brief Check if redo is available.
     * @return True if a redo operation is possible.
     */
    bool canRedo() const;

    /**
     * @brief Get the text of the command that would be undone.
     * @return Text description.
     */
    QString undoText() const;

    /**
     * @brief Get the text of the command that would be redone.
     * @return Text description.
     */
    QString redoText() const;

    /**
     * @brief Get the number of commands in the undo stack.
     * @return Count of undoable commands.
     */
    int undoStackSize() const;

    /**
     * @brief Get the number of commands in the redo stack.
     * @return Count of redoable commands.
     */
    int redoStackSize() const;

    /**
     * @brief Set the maximum number of commands the stack can hold.
     * @param limit Maximum command count (0 means unlimited).
     */
    void setUndoLimit(int limit);

    /**
     * @brief Get the current undo limit.
     * @return Maximum command count.
     */
    int undoLimit() const;

    /**
     * @brief Begin a macro command group.
     * Commands pushed after this call will be grouped together.
     * @param text Name for the macro.
     */
    void beginMacro(const QString& text);

    /**
     * @brief End the current macro command group.
     */
    void endMacro();

    /**
     * @brief Get the associated document.
     * @return Pointer to the document this stack operates on.
     */
    Document* document() const;

    /**
     * @brief Set the associated document.
     * @param doc The document this stack operates on.
     */
    void setDocument(Document* doc);

signals:
    /**
     * @brief Emitted when undo/redo availability changes.
     */
    void canUndoChanged();
    void canRedoChanged();

    /**
     * @brief Emitted when undo/redo text changes.
     */
    void undoTextChanged(const QString& text);
    void redoTextChanged(const QString& text);

    /**
     * @brief Emitted when the stack is cleaned (e.g., after save).
     */
    void cleanChanged();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_UNDOSTACK_H