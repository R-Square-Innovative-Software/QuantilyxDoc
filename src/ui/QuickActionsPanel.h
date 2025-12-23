/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_QUICKACTIONSPANEL_H
#define QUANTILYX_QUICKACTIONSPANEL_H

#include <QDockWidget>
#include <QList>
#include <QPair>
#include <QMutex>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

/**
 * @brief Represents a single quick action button.
 */
struct QuickAction {
    QString id;             // Unique identifier for the action
    QString title;          // Display name of the action
    QString description;    // Brief description
    QIcon icon;            // Icon for the button
    std::function<void()> handler; // Function to execute when clicked
    bool isFavorite;       // Whether the user has marked this as a favorite
    int usageCount;        // Number of times this action has been used (for adaptive UI)
    QDateTime lastUsed;    // When the action was last used (for adaptive UI)
};

/**
 * @brief A panel providing quick access to frequently used or important actions.
 * 
 * Displays a set of customizable buttons for common tasks like opening files,
 * saving, undo/redo, zooming, printing, etc. Can adapt its content based
 * on usage patterns or user preferences.
 */
class QuickActionsPanel : public QDockWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent widget (usually the main window).
     */
    explicit QuickActionsPanel(QWidget* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~QuickActionsPanel() override;

    /**
     * @brief Add an action to the panel.
     * @param id Unique identifier for the action.
     * @param title Display title.
     * @param description Description.
     * @param icon Icon.
     * @param handler Function to execute.
     * @param isFavorite Whether to mark as favorite initially.
     */
    void addAction(const QString& id,
                   const QString& title,
                   const QString& description,
                   const QIcon& icon,
                   std::function<void()> handler,
                   bool isFavorite = false);

    /**
     * @brief Remove an action from the panel by its ID.
     * @param id The ID of the action to remove.
     */
    void removeAction(const QString& id);

    /**
     * @brief Get the list of all actions currently managed by the panel.
     * @return List of QuickAction structures.
     */
    QList<QuickAction> actions() const;

    /**
     * @brief Mark an action as favorite or not.
     * @param id The ID of the action.
     * @param favorite Whether to mark as favorite.
     */
    void setActionAsFavorite(const QString& id, bool favorite);

    /**
     * @brief Check if an action is marked as a favorite.
     * @param id The ID of the action.
     * @return True if favorite.
     */
    bool isActionFavorite(const QString& id) const;

    /**
     * @brief Promote an action as frequently used.
     * This increases its usage count and updates its last used time.
     * Can be used by the UI to prioritize frequent actions.
     * @param id The ID of the action.
     */
    void promoteActionAsFrequent(const QString& id);

    /**
     * @brief Get the number of times an action has been used.
     * @param id The ID of the action.
     * @return Usage count.
     */
    int actionUsageCount(const QString& id) const;

    /**
     * @brief Get the time an action was last used.
     * @param id The ID of the action.
     * @return Last used time.
     */
    QDateTime actionLastUsed(const QString& id) const;

    /**
     * @brief Set the maximum number of actions to display simultaneously.
     * @param maxCount Maximum number of actions.
     */
    void setMaxVisibleActions(int maxCount);

    /**
     * @brief Get the maximum number of actions to display simultaneously.
     * @return Maximum number of actions.
     */
    int maxVisibleActions() const;

    /**
     * @brief Get the current layout style (e.g., icons only, text only, icons and text).
     * @return Layout style string.
     */
    QString layoutStyle() const;

    /**
     * @brief Set the current layout style.
     * @param style Layout style string (e.g., "icons_only", "text_only", "icons_and_text").
     */
    void setLayoutStyle(const QString& style);

    /**
     * @brief Check if the panel is configured to adapt its content based on usage.
     * @return True if adaptive mode is enabled.
     */
    bool isAdaptiveMode() const;

    /**
     * @brief Enable or disable adaptive mode.
     * In adaptive mode, the panel might reorder actions based on usage frequency
     * or hide/show actions based on context.
     * @param adaptive True to enable adaptive mode.
     */
    void setAdaptiveMode(bool adaptive);

    /**
     * @brief Get the list of supported layout styles.
     * @return List of style strings.
     */
    QStringList supportedLayoutStyles() const;

signals:
    /**
     * @brief Emitted when an action is added to the panel.
     * @param actionId The ID of the added action.
     */
    void actionAdded(const QString& actionId);

    /**
     * @brief Emitted when an action is removed from the panel.
     * @param actionId The ID of the removed action.
     */
    void actionRemoved(const QString& actionId);

    /**
     * @brief Emitted when an action is executed via the panel.
     * @param actionId The ID of the executed action.
     */
    void actionExecuted(const QString& actionId);

    /**
     * @brief Emitted when the favorite status of an action changes.
     * @param actionId The ID of the action.
     * @param isFavorite The new favorite status.
     */
    void actionFavoriteChanged(const QString& actionId, bool isFavorite);

    /**
     * @brief Emitted when the visibility or arrangement of actions changes due to adaptive UI or user reordering.
     */
    void actionsLayoutChanged();

private:
    class Private;
    std::unique_ptr<Private> d;

    // Helper to update the UI based on the current list of actions and settings
    void updateUi();
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_QUICKACTIONSPANEL_H