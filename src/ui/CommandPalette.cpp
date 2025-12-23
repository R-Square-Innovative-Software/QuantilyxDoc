/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "CommandPalette.h"
#include "../core/Settings.h"
#include "../core/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QKeyEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QCompleter>
#include <QStringListModel>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QSortFilterProxyModel>
#include <QDebug>

namespace QuantilyxDoc {

class CommandPalette::Private {
public:
    Private(CommandPalette* q_ptr)
        : q(q_ptr), maxResultsVal(15), closeOnExecuteVal(true), isShownVal(false) {}

    CommandPalette* q;
    QLineEdit* searchLineEdit;
    QListWidget* resultsListWidget;
    QLabel* placeholderLabel;
    QList<Command> allCommands;
    QHash<QString, Command> commandMap; // For fast lookup by ID
    QStringListModel* listModel; // Model for the list widget
    QSortFilterProxyModel* proxyModel; // Proxy model for filtering
    QTimer* searchDebounceTimer; // Timer to debounce search as user types
    int maxResultsVal;
    bool closeOnExecuteVal;
    bool isShownVal;
    QString currentQueryStr;
    mutable QMutex mutex; // Protect access to command list during async operations

    // Helper to filter commands based on the current query string
    void filterCommands(const QString& query) {
        QMutexLocker locker(&mutex); // Lock during read of command list and filtering
        QStringList results;

        if (query.isEmpty()) {
            // If query is empty, show top-level categories or most frequent commands
            QSet<QString> categories;
            for (const auto& cmd : allCommands) {
                categories.insert(cmd.category);
            }
            for (const QString& cat : categories) {
                results.append(cat + " (Category)");
            }
        } else {
            // Simple fuzzy search across title, category, description
            QString lowerQuery = query.toLower();
            QList<Command> matchedCommands;
            for (const auto& cmd : allCommands) {
                if (cmd.title.toLower().contains(lowerQuery) ||
                    cmd.category.toLower().contains(lowerQuery) ||
                    cmd.description.toLower().contains(lowerQuery)) {
                    matchedCommands.append(cmd);
                }
            }

            // Sort results: prioritize by priority, then by title
            std::sort(matchedCommands.begin(), matchedCommands.end(), [](const Command& a, const Command& b) {
                if (a.priority != b.priority) {
                    return a.priority > b.priority; // Higher priority first
                }
                return a.title.localeAwareCompare(b.title) < 0; // Alphabetical
            });

            // Take up to maxResults
            int count = qMin(matchedCommands.size(), maxResultsVal);
            for (int i = 0; i < count; ++i) {
                const Command& cmd = matchedCommands[i];
                // Format result item text
                QString itemText = QString("%1 (%2)").arg(cmd.title).arg(cmd.category);
                if (!cmd.shortcut.isEmpty()) {
                    itemText += QString(" [%1]").arg(cmd.shortcut);
                }
                results.append(itemText);
            }
        }

        // Set the filtered results to the proxy model
        static_cast<QStringListModel*>(proxyModel->sourceModel())->setStringList(results);

        // Update the internal list of *matched* commands for execution mapping
        // This needs a better design - the model should ideally store Command structs, not just strings.
        // For now, we'll rebuild a map of *filtered* commands on the fly.
        // A more robust approach uses a custom QAbstractListModel storing Command objects.
        currentFilteredCommands.clear();
        if (query.isEmpty()) {
             // Show categories - this logic needs refinement. For now, add all commands if query is empty.
             currentFilteredCommands = allCommands;
        } else {
             // Use the matchedCommands list from above
             // (This logic is duplicated from filtering, inefficient but simple for now)
             QString lowerQuery = query.toLower();
             for (const auto& cmd : allCommands) {
                 if (cmd.title.toLower().contains(lowerQuery) ||
                     cmd.category.toLower().contains(lowerQuery) ||
                     cmd.description.toLower().contains(lowerQuery)) {
                     currentFilteredCommands.append(cmd);
                 }
             }
             // Apply same sorting as above
             std::sort(currentFilteredCommands.begin(), currentFilteredCommands.end(), [](const Command& a, const Command& b) {
                 if (a.priority != b.priority) {
                     return a.priority > b.priority;
                 }
                 return a.title.localeAwareCompare(b.title) < 0;
             });
             currentFilteredCommands = currentFilteredCommands.mid(0, maxResultsVal);
        }

        emit q->resultsChanged(results.size());
        LOG_DEBUG("CommandPalette: Filtered to " << results.size() << " commands for query: '" << query << "'");
    }

    // List of currently filtered commands (subset of allCommands matching the query)
    QList<Command> currentFilteredCommands;
};

// Static instance pointer
CommandPalette* CommandPalette::s_instance = nullptr;

CommandPalette& CommandPalette::instance()
{
    if (!s_instance) {
        s_instance = new CommandPalette();
    }
    return *s_instance;
}

CommandPalette::CommandPalette(QWidget* parent)
    : QWidget(parent, Qt::Popup) // Make it a popup window
    , d(new Private(this))
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint); // Ensure it behaves like a palette
    setAttribute(Qt::WA_TranslucentBackground); // Allow custom background painting if desired

    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Search input
    d->searchLineEdit = new QLineEdit(this);
    d->searchLineEdit->setPlaceholderText(tr("Type a command..."));
    d->searchLineEdit->setClearButtonEnabled(true);
    mainLayout->addWidget(d->searchLineEdit);

    // Results list
    d->listModel = new QStringListModel(this); // Simple string list for now
    d->proxyModel = new QSortFilterProxyModel(this);
    d->proxyModel->setSourceModel(d->listModel);
    // d->proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive); // Filter is done manually in filterCommands
    // d->proxyModel->setFilterKeyColumn(-1); // Filter all columns (or just 0 if using custom model)

    d->resultsListWidget = new QListWidget(this);
    d->resultsListWidget->setModel(d->proxyModel);
    d->resultsListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainLayout->addWidget(d->resultsListWidget);

    // Placeholder label (shown when no results)
    d->placeholderLabel = new QLabel(tr("No commands found"), this);
    d->placeholderLabel->setAlignment(Qt::AlignCenter);
    d->placeholderLabel->hide(); // Hidden initially, shown by filterResults if necessary
    mainLayout->addWidget(d->placeholderLabel);

    // Debounce timer for search
    d->searchDebounceTimer = new QTimer(this);
    d->searchDebounceTimer->setSingleShot(true);
    connect(d->searchDebounceTimer, &QTimer::timeout, [this]() {
        d->filterCommands(d->searchLineEdit->text());
    });

    // Connect search box changes
    connect(d->searchLineEdit, &QLineEdit::textChanged, [this](const QString& text) {
        d->currentQueryStr = text;
        d->searchDebounceTimer->start(150); // Wait 150ms after user stops typing
        emit queryChanged(text);
    });

    // Connect list item activation (click or Enter)
    connect(d->resultsListWidget, &QListWidget::activated, [this](const QModelIndex& index) {
        int sourceRow = d->proxyModel->mapToSource(index).row();
        if (sourceRow >= 0 && sourceRow < d->currentFilteredCommands.size()) {
            const Command& cmd = d->currentFilteredCommands[sourceRow];
            executeCommand(cmd);
        }
    });

    // Connect Enter key press in search box to activate selected item
    connect(d->searchLineEdit, &QLineEdit::returnPressed, [this]() {
        if (d->resultsListWidget->currentIndex().isValid()) {
            // Trigger the activated signal for the current index
            d->resultsListWidget->activated(d->resultsListWidget->currentIndex());
        } else {
            // If no item is selected, try to execute the first result if available
            if (!d->currentFilteredCommands.isEmpty()) {
                executeCommand(d->currentFilteredCommands[0]);
            }
        }
    });

    // Connect Escape key to hide the palette
    connect(d->searchLineEdit, &QLineEdit::keyPressed, [this](QKeyEvent* event) {
        if (event->key() == Qt::Key_Escape) {
            hidePalette();
            event->accept();
        }
    });

    // Initially hide the palette
    hide();

    LOG_INFO("CommandPalette initialized.");
}

CommandPalette::~CommandPalette()
{
    LOG_INFO("CommandPalette destroyed.");
}

void CommandPalette::showPalette()
{
    // Center on parent or primary screen
    QWidget* parentWidget = parentWidget();
    QRect screenGeometry;
    if (parentWidget) {
        screenGeometry = parentWidget->screen()->availableGeometry();
    } else {
        screenGeometry = QApplication::primaryScreen()->availableGeometry();
    }

    int x = screenGeometry.center().x() - width() / 2;
    int y = screenGeometry.center().y() - height() / 2;
    move(x, y);

    show();
    d->isShownVal = true;
    d->searchLineEdit->setFocus();
    d->searchLineEdit->selectAll(); // Clear any existing text or select all for easy replacement
    emit paletteShown();
    LOG_DEBUG("CommandPalette shown.");
}

void CommandPalette::hidePalette()
{
    hide();
    d->isShownVal = false;
    // Optionally, return focus to the previous widget
    if (QWidget* previousFocus = QApplication::focusWidget()) {
        previousFocus->setFocus();
    }
    emit paletteHidden();
    LOG_DEBUG("CommandPalette hidden.");
}

bool CommandPalette::isShown() const
{
    return d->isShownVal;
}

void CommandPalette::addCommand(const QString& id, const QString& title, const QString& category, const QString& description, const QString& shortcut, std::function<void()> handler, const QIcon& icon, int priority)
{
    QMutexLocker locker(&d->mutex);

    // Check for duplicates
    auto existingCmdIt = std::find_if(d->allCommands.begin(), d->allCommands.end(),
                                      [&id](const Command& cmd) { return cmd.id == id; });
    if (existingCmdIt != d->allCommands.end()) {
        LOG_WARN("CommandPalette::addCommand: Command with ID already exists, overwriting: " << id);
        *existingCmdIt = Command{id, title, category, description, shortcut, std::move(handler), icon, priority};
    } else {
        d->allCommands.append(Command{id, title, category, description, shortcut, std::move(handler), icon, priority});
        d->commandMap.insert(id, d->allCommands.back()); // Update map
    }

    // If the palette is currently visible and the query matches, update the list
    if (isShown() && d->currentQueryStr.isEmpty()) { // Simple update if query is empty
        d->filterCommands(d->currentQueryStr);
    }

    LOG_DEBUG("CommandPalette: Added command '" << title << "' (ID: " << id << ")");
}

void CommandPalette::removeCommand(const QString& id)
{
    QMutexLocker locker(&d->mutex);

    auto it = std::find_if(d->allCommands.begin(), d->allCommands.end(),
                           [&id](const Command& cmd) { return cmd.id == id; });
    if (it != d->allCommands.end()) {
        d->allCommands.erase(it);
        d->commandMap.remove(id);
        LOG_DEBUG("CommandPalette: Removed command (ID: " << id << ")");

        // Update list if visible
        if (isShown()) {
            d->filterCommands(d->currentQueryStr);
        }
    } else {
        LOG_WARN("CommandPalette::removeCommand: Command ID not found: " << id);
    }
}

QList<Command> CommandPalette::allCommands() const
{
    QMutexLocker locker(&d->mutex);
    return d->allCommands; // Returns a copy
}

QList<Command> CommandPalette::searchCommands(const QString& query) const
{
    QMutexLocker locker(&d->mutex);
    // Re-implement the search logic from filterCommands to return the actual Command objects
    QList<Command> results;
    QString lowerQuery = query.toLower();

    for (const auto& cmd : d->allCommands) {
        if (cmd.title.toLower().contains(lowerQuery) ||
            cmd.category.toLower().contains(lowerQuery) ||
            cmd.description.toLower().contains(lowerQuery)) {
            results.append(cmd);
        }
    }

    // Sort results: prioritize by priority, then by title
    std::sort(results.begin(), results.end(), [](const Command& a, const Command& b) {
        if (a.priority != b.priority) {
            return a.priority > b.priority;
        }
        return a.title.localeAwareCompare(b.title) < 0;
    });

    // Limit results
    if (results.size() > d->maxResultsVal) {
        results = results.mid(0, d->maxResultsVal);
    }

    return results;
}

void CommandPalette::clearCommands()
{
    QMutexLocker locker(&d->mutex);
    d->allCommands.clear();
    d->commandMap.clear();
    d->currentFilteredCommands.clear();
    d->listModel->setStringList(QStringList()); // Clear the UI model
    LOG_DEBUG("CommandPalette: Cleared all commands.");
}

QString CommandPalette::currentQuery() const
{
    QMutexLocker locker(&d->mutex);
    return d->currentQueryStr;
}

void CommandPalette::setMaxResults(int maxCount)
{
    if (maxCount > 0) {
        QMutexLocker locker(&d->mutex);
        if (d->maxResultsVal != maxCount) {
            d->maxResultsVal = maxCount;
            LOG_INFO("CommandPalette: Max results set to " << maxCount);
            if (isShown()) {
                d->filterCommands(d->currentQueryStr); // Re-filter with new limit
            }
        }
    }
}

int CommandPalette::maxResults() const
{
    QMutexLocker locker(&d->mutex);
    return d->maxResultsVal;
}

void CommandPalette::setCloseOnExecute(bool close)
{
    QMutexLocker locker(&d->mutex);
    d->closeOnExecuteVal = close;
    LOG_DEBUG("CommandPalette: Close on execute set to " << close);
}

bool CommandPalette::closeOnExecute() const
{
    QMutexLocker locker(&d->mutex);
    return d->closeOnExecuteVal;
}

void CommandPalette::executeCommandById(const QString& commandId)
{
    QMutexLocker locker(&d->mutex);
    auto it = d->commandMap.constFind(commandId);
    if (it != d->commandMap.constEnd()) {
        executeCommand(it.value());
    } else {
        LOG_WARN("CommandPalette::executeCommandById: Command ID not found: " << commandId);
    }
}

void CommandPalette::executeCommand(const Command& cmd)
{
    LOG_INFO("CommandPalette: Executing command '" << cmd.title << "' (ID: " << cmd.id << ")");
    if (cmd.handler) {
        cmd.handler(); // Execute the associated function
    }
    emit commandExecuted(cmd.id);

    if (d->closeOnExecuteVal) {
        hidePalette(); // Close the palette after execution if configured
    }
}

void CommandPalette::onSearchEditTextChanged(const QString& text)
{
    // Handled by the signal connection to the debounce timer in constructor
    Q_UNUSED(text);
}

void CommandPalette::onResultListCurrentItemChanged()
{
    // Could highlight the command details in a separate area if added
}

void CommandPalette::onResultListActivated()
{
    // Handled by the signal connection in constructor
}

void CommandPalette::keyPressEvent(QKeyEvent* event)
{
    // Handle navigation keys if the list has focus but search box doesn't
    if (event->key() == Qt::Key_Escape) {
        hidePalette();
        event->accept();
        return;
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // Handled by QLineEdit's returnPressed signal and QListWidget's activated signal
        // QWidget::keyPressEvent(event); // Let base class handle it if needed elsewhere
        return;
    } else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        // Let the QListWidget handle navigation
        d->resultsListWidget->setFocus(); // Ensure list handles the keys
        // QWidget::keyPressEvent(event); // Let base class handle it
        return;
    }
    QWidget::keyPressEvent(event);
}

void CommandPalette::paintEvent(QPaintEvent* event)
{
    // Paint a custom background if desired (e.g., rounded corners, shadow)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background rect with rounded corners
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    QWidget::paintEvent(event);
}

} // namespace QuantilyxDoc