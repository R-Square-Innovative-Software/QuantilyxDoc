/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "QuickActionsPanel.h"
#include "../core/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QToolButton>
#include <QLabel>
#include <QScrollArea>
#include <QButtonGroup>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QDebug>

namespace QuantilyxDoc {

class QuickActionsPanel::Private {
public:
    Private(QuickActionsPanel* q_ptr)
        : q(q_ptr), maxVisibleActionsVal(10), adaptiveModeVal(false) {}

    QuickActionsPanel* q;
    QScrollArea* scrollArea;
    QWidget* contentWidget;
    QGridLayout* contentLayout;
    QButtonGroup* buttonGroup;
    QList<QuickAction> actions;
    QHash<QString, int> actionIdToIndex; // Map ID -> index in actions list for quick lookup
    int maxVisibleActionsVal;
    bool adaptiveModeVal;
    mutable QMutex mutex; // Protect access to the actions list during updates

    // Helper to update the UI based on the current list of actions and visibility settings
    void updateUi() {
        QMutexLocker locker(&mutex); // Lock during UI update

        // Clear existing buttons from layout (but don't delete the QToolButtons themselves yet)
        QLayoutItem* child;
        while ((child = contentLayout->takeAt(0)) != nullptr) {
            delete child->widget(); // Delete the actual QToolButton widget
            delete child;          // Delete the layout item
        }

        // Determine which actions to show
        // If adaptive, sort by usage count or last used time and pick top N
        // If not adaptive, show all favorites first, then others up to max.
        QList<QuickAction> actionsToShow;
        if (adaptiveModeVal) {
            // Create a copy and sort by usage/frequency/last used
            QList<QuickAction> sortedActions = actions;
            std::sort(sortedActions.begin(), sortedActions.end(), [](const QuickAction& a, const QuickAction& b) {
                // Prioritize by usage count first, then by last used time
                if (a.usageCount != b.usageCount) {
                    return a.usageCount > b.usageCount;
                }
                return a.lastUsed > b.lastUsed;
            });
            actionsToShow = sortedActions.mid(0, maxVisibleActionsVal);
        } else {
            // Show favorites first, then others
            for (const auto& action : actions) {
                if (action.isFavorite) {
                    actionsToShow.append(action);
                }
            }
            int remainingSlots = maxVisibleActionsVal - actionsToShow.size();
            if (remainingSlots > 0) {
                for (const auto& action : actions) {
                    if (!action.isFavorite && actionsToShow.size() < maxVisibleActionsVal) {
                        actionsToShow.append(action);
                    }
                }
            }
        }

        // Create and add buttons for the actions to show
        int rows = 0;
        int cols = 4; // Fixed number of columns for grid layout
        int currentRow = 0;
        int currentCol = 0;

        for (const auto& action : actionsToShow) {
            QToolButton* button = new QToolButton(q);
            button->setIcon(action.icon);
            button->setIconSize(QSize(24, 24)); // Standard icon size for quick actions
            button->setText(action.title);
            button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); // Icon above text
            button->setToolTip(action.description); // Show description as tooltip
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            button->setMinimumHeight(50); // Ensure buttons are tall enough

            // Store the action ID in the button's object name for identification
            button->setObjectName("QuickActionBtn_" + action.id);

            // Add to layout
            contentLayout->addWidget(button, currentRow, currentCol);
            currentCol++;
            if (currentCol >= cols) {
                currentCol = 0;
                currentRow++;
            }

            // Connect button click to the action's handler
            connect(button, &QToolButton::clicked, [this, id = action.id]() {
                executeAction(id);
            });
        }

        // Add stretch item to push buttons towards the top-left if fewer than grid capacity
        contentLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding), currentRow, 0, 1, cols);

        LOG_DEBUG("QuickActionsPanel: Updated UI with " << actionsToShow.size() << " visible actions.");
    }

    // Helper to execute an action by its ID
    void executeAction(const QString& id) {
        QMutexLocker locker(&mutex);
        auto it = actionIdToIndex.constFind(id);
        if (it != actionIdToIndex.constEnd()) {
            int index = it.value();
            if (index >= 0 && index < actions.size()) {
                QuickAction& action = actions[index];
                LOG_INFO("QuickActionsPanel: Executing quick action '" << action.title << "' (ID: " << action.id << ")");
                if (action.handler) {
                    action.handler(); // Execute the associated function
                }
                // Update usage statistics
                action.usageCount++;
                action.lastUsed = QDateTime::currentDateTime();
                emit q->actionExecuted(id);

                // If adaptive mode is on, the UI might need a slight delay/update to reflect new usage stats
                if (adaptiveModeVal) {
                    // Use a single shot timer to avoid blocking the UI thread during button click
                    QTimer::singleShot(0, [this]() { updateUi(); });
                }
            }
        } else {
            LOG_WARN("QuickActionsPanel::executeAction: Action ID not found: " << id);
        }
    }
};

QuickActionsPanel::QuickActionsPanel(QWidget* parent)
    : QDockWidget(parent)
    , d(new Private(this))
{
    setObjectName("QuickActionsPanel"); // Essential for QMainWindow to save/restore state
    setWindowTitle(tr("Quick Actions")); // Default title, can be changed by user
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea); // Usually on the sides

    // Central widget for the dock
    QWidget* centralWidget = new QWidget(this);

    // Scroll area to hold the action buttons
    d->scrollArea = new QScrollArea(centralWidget);
    d->scrollArea->setWidgetResizable(true);
    d->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Vertical scroll only for grid
    d->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Content widget and grid layout
    d->contentWidget = new QWidget(d->scrollArea);
    d->contentLayout = new QGridLayout(d->contentWidget);
    d->contentLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft); // Align buttons to top-left
    d->contentLayout->setSpacing(5); // Space between buttons
    d->contentLayout->setContentsMargins(5, 5, 5, 5); // Padding around grid

    d->scrollArea->setWidget(d->contentWidget);

    // Main layout for central widget
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(d->scrollArea);
    mainLayout->setContentsMargins(0, 0, 0, 0); // Let scroll area handle its own margins

    setWidget(centralWidget);

    // Button group (not strictly necessary for QToolButton if not mutually exclusive, but good practice)
    d->buttonGroup = new QButtonGroup(this);
    d->buttonGroup->setExclusive(false); // Actions are independent

    LOG_INFO("QuickActionsPanel initialized.");
}

QuickActionsPanel::~QuickActionsPanel()
{
    LOG_INFO("QuickActionsPanel destroyed.");
}

void QuickActionsPanel::addAction(const QString& id, const QString& title, const QString& description, const QIcon& icon, std::function<void()> handler, bool isFavorite)
{
    QMutexLocker locker(&d->mutex);

    // Check for duplicates
    auto existingIt = std::find_if(d->actions.begin(), d->actions.end(),
                                   [&id](const QuickAction& act) { return act.id == id; });
    if (existingIt != d->actions.end()) {
        LOG_WARN("QuickActionsPanel::addAction: Action with ID already exists, overwriting: " << id);
        int index = std::distance(d->actions.begin(), existingIt);
        d->actionIdToIndex.remove(existingIt->id); // Remove old ID mapping
        *existingIt = QuickAction{id, title, description, icon, std::move(handler), isFavorite, 0, QDateTime()}; // Replace
        d->actionIdToIndex.insert(id, index); // Add new ID mapping
    } else {
        int newIndex = d->actions.size();
        d->actions.append(QuickAction{id, title, description, icon, std::move(handler), isFavorite, 0, QDateTime()});
        d->actionIdToIndex.insert(id, newIndex);
    }

    LOG_DEBUG("QuickActionsPanel: Added action '" << title << "' (ID: " << id << ", Favorite: " << isFavorite << ")");

    // Update UI if dock is visible or adaptive mode is on
    if (isVisible() || d->adaptiveModeVal) {
        d->updateUi();
    }
}

void QuickActionsPanel::removeAction(const QString& id)
{
    QMutexLocker locker(&d->mutex);

    auto it = std::find_if(d->actions.begin(), d->actions.end(),
                           [&id](const QuickAction& act) { return act.id == id; });
    if (it != d->actions.end()) {
        int index = std::distance(d->actions.begin(), it);
        d->actionIdToIndex.remove(it->id);
        d->actions.erase(it);

        // Shift indices in the map for items after the removed one
        for (auto mapIt = d->actionIdToIndex.begin(); mapIt != d->actionIdToIndex.end(); ++mapIt) {
            if (mapIt.value() > index) {
                mapIt.value()--; // Decrement index
            }
        }

        LOG_DEBUG("QuickActionsPanel: Removed action (ID: " << id << ")");

        // Update UI
        d->updateUi();
    } else {
        LOG_WARN("QuickActionsPanel::removeAction: Action ID not found: " << id);
    }
}

QList<QuickAction> QuickActionsPanel::actions() const
{
    QMutexLocker locker(&d->mutex);
    return d->actions; // Returns a copy
}

void QuickActionsPanel::setActionAsFavorite(const QString& id, bool favorite)
{
    QMutexLocker locker(&d->mutex);
    auto it = std::find_if(d->actions.begin(), d->actions.end(),
                           [&id](const QuickAction& act) { return act.id == id; });
    if (it != d->actions.end()) {
        it->isFavorite = favorite;
        LOG_DEBUG("QuickActionsPanel: Set action '" << id << "' as favorite: " << favorite);

        // Update UI if dock is visible or adaptive mode is on
        if (isVisible() || d->adaptiveModeVal) {
            d->updateUi();
        }
    } else {
        LOG_WARN("QuickActionsPanel::setActionAsFavorite: Action ID not found: " << id);
    }
}

bool QuickActionsPanel::isActionFavorite(const QString& id) const
{
    QMutexLocker locker(&d->mutex);
    auto it = std::find_if(d->actions.begin(), d->actions.end(),
                           [&id](const QuickAction& act) { return act.id == id; });
    return (it != d->actions.end() && it->isFavorite);
}

void QuickActionsPanel::promoteActionAsFrequent(const QString& id)
{
    QMutexLocker locker(&d->mutex);
    auto it = std::find_if(d->actions.begin(), d->actions.end(),
                           [&id](const QuickAction& act) { return act.id == id; });
    if (it != d->actions.end()) {
        it->usageCount++;
        it->lastUsed = QDateTime::currentDateTime();
        LOG_DEBUG("QuickActionsPanel: Promoted action '" << id << "' as frequent (usage: " << it->usageCount << ").");

        // If adaptive mode is on, UI might update automatically based on usage
        if (d->adaptiveModeVal && isVisible()) {
            d->updateUi(); // Refresh to reflect new priority
        }
    } else {
        LOG_WARN("QuickActionsPanel::promoteActionAsFrequent: Action ID not found: " << id);
    }
}

int QuickActionsPanel::actionUsageCount(const QString& id) const
{
    QMutexLocker locker(&d->mutex);
    auto it = std::find_if(d->actions.begin(), d->actions.end(),
                           [&id](const QuickAction& act) { return act.id == id; });
    return (it != d->actions.end()) ? it->usageCount : 0;
}

QDateTime QuickActionsPanel::actionLastUsed(const QString& id) const
{
    QMutexLocker locker(&d->mutex);
    auto it = std::find_if(d->actions.begin(), d->actions.end(),
                           [&id](const QuickAction& act) { return act.id == id; });
    return (it != d->actions.end()) ? it->lastUsed : QDateTime();
}

void QuickActionsPanel::setMaxVisibleActions(int maxCount)
{
    if (maxCount > 0) {
        QMutexLocker locker(&d->mutex);
        if (d->maxVisibleActionsVal != maxCount) {
            d->maxVisibleActionsVal = maxCount;
            LOG_INFO("QuickActionsPanel: Max visible actions set to " << maxCount);

            // Update UI if dock is visible or adaptive mode is on
            if (isVisible() || d->adaptiveModeVal) {
                d->updateUi();
            }
        }
    }
}

int QuickActionsPanel::maxVisibleActions() const
{
    QMutexLocker locker(&d->mutex);
    return d->maxVisibleActionsVal;
}

QString QuickActionsPanel::layoutStyle() const
{
    // This could return a predefined style name like "icons_only", "text_only", "icons_and_text_grid", "icons_and_text_flow"
    // For now, let's say it's a grid with icons and text.
    return "icons_and_text_grid";
}

void QuickActionsPanel::setLayoutStyle(const QString& style)
{
    // This would change how the buttons are arranged and what is shown (icon/text/both).
    // Implementation depends on the specific styles supported.
    LOG_WARN("QuickActionsPanel::setLayoutStyle: Not implemented. Current style is fixed grid with icon and text.");
    Q_UNUSED(style);
}

bool QuickActionsPanel::isAdaptiveMode() const
{
    QMutexLocker locker(&d->mutex);
    return d->adaptiveModeVal;
}

void QuickActionsPanel::setAdaptiveMode(bool adaptive)
{
    QMutexLocker locker(&d->mutex);
    if (d->adaptiveModeVal != adaptive) {
        d->adaptiveModeVal = adaptive;
        LOG_INFO("QuickActionsPanel: Adaptive mode set to " << adaptive);

        // Update UI immediately to reflect new behavior
        d->updateUi();
    }
}

QStringList QuickActionsPanel::supportedLayoutStyles() const
{
    return QStringList() << "icons_and_text_grid" << "icons_only_grid" << "text_only_list";
}

void QuickActionsPanel::updateUi()
{
    d->updateUi(); // Delegate to private implementation
}

} // namespace QuantilyxDoc