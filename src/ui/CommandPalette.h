/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_COMMANDPALETTE_H
#define QUANTILYX_COMMANDPALETTE_H

#include <QWidget>
#include <QList>
#include <QPair>
#include <QMutex>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

/**
 * @brief Represents a single command available in the command palette.
 */
struct Command {
    QString id;             // Unique identifier for the command
    QString title;          // Display name of the command
    QString category;       // Category for grouping (e.g., "File", "Edit", "View", "Tools")
    QString description;    // Brief description of what the command does
    QString shortcut;       // Associated keyboard shortcut (e.g., "Ctrl+S")
    std::function<void()> handler; // Function to execute when command is selected
    QIcon icon;            // Optional icon for the command
    int priority;          // Priority for sorting in results (higher = higher up)
};

/**
 * @brief A searchable command palette for quick access to application features.
 * 
 * Provides a modal dialog or popup window where users can type keywords
 * to find and execute commands, menus, or actions within the application.
 * Often activated by a global hotkey (e.g., Ctrl+Shift+P).
 */
class CommandPalette : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent widget (usually the main window).
     */
    explicit CommandPalette(QWidget* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~CommandPalette() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global CommandPalette instance.
     */
    static CommandPalette& instance();

    /**
     * @brief Show the command palette.
     * Centers itself over the parent widget.
     */
    void showPalette();

    /**
     * @brief Hide the command palette.
     */
    void hidePalette();

    /**
     * @brief Check if the palette is currently visible.
     * @return True if visible.
     */
    bool isShown() const;

    /**
     * @brief Add a command to the palette's registry.
     * @param id Unique identifier for the command.
     * @param title Display title.
     * @param category Category for grouping.
     * @param description Description.
     * @param shortcut Keyboard shortcut.
     * @param handler Function to execute.
     * @param icon Optional icon.
     * @param priority Sorting priority.
     */
    void addCommand(const QString& id,
                    const QString& title,
                    const QString& category,
                    const QString& description,
                    const QString& shortcut,
                    std::function<void()> handler,
                    const QIcon& icon = QIcon(),
                    int priority = 0);

    /**
     * @brief Remove a command from the palette's registry by its ID.
     * @param id The ID of the command to remove.
     */
    void removeCommand(const QString& id);

    /**
     * @brief Get the list of all registered commands.
     * @return List of Command structures.
     */
    QList<Command> allCommands() const;

    /**
     * @brief Get the list of commands matching a search query.
     * @param query Search string.
     * @return List of matching Command structures.
     */
    QList<Command> searchCommands(const QString& query) const;

    /**
     * @brief Clear all registered commands.
     */
    void clearCommands();

    /**
     * @brief Get the current search query string.
     * @return Current query.
     */
    QString currentQuery() const;

    /**
     * @brief Set the maximum number of results to display.
     * @param maxCount Maximum number of results.
     */
    void setMaxResults(int maxCount);

    /**
     * @brief Get the maximum number of results to display.
     * @return Maximum number of results.
     */
    int maxResults() const;

    /**
     * @brief Set whether the palette should close after a command is executed.
     * @param closeOnExecute True to close.
     */
    void setCloseOnExecute(bool closeOnExecute);

    /**
     * @brief Get whether the palette closes after a command is executed.
     * @return True if closes on execute.
     */
    bool closeOnExecute() const;

signals:
    /**
     * @brief Emitted when the palette is shown.
     */
    void paletteShown();

    /**
     * @brief Emitted when the palette is hidden.
     */
    void paletteHidden();

    /**
     * @brief Emitted when a command is executed via the palette.
     * @param commandId The ID of the executed command.
     */
    void commandExecuted(const QString& commandId);

    /**
     * @brief Emitted when the search query changes.
     * @param query The new query string.
     */
    void queryChanged(const QString& query);

    /**
     * @brief Emitted when the list of results changes.
     * @param resultCount The number of results found.
     */
    void resultsChanged(int resultCount);

public slots:
    /**
     * @brief Execute a specific command by its ID.
     * @param commandId The ID of the command to execute.
     */
    void executeCommandById(const QString& commandId);

private slots:
    void onSearchEditTextChanged(const QString& text);
    void onResultListCurrentItemChanged();
    void onResultListActivated();

private:
    class Private;
    std::unique_ptr<Private> d;

    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_COMMANDPALETTE_H