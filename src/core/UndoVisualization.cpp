/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "UndoVisualization.h"
#include "UndoStack.h"
#include "Document.h"
#include "Logger.h"
#include <QUndoStack>
#include <QUndoCommand>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QImage>
#include <QApplication> // For thumbnail generation context
#include <QThread>      // To check if running on main thread

namespace QuantilyxDoc {

class UndoVisualization::Private {
public:
    Private(UndoVisualization* q_ptr)
        : q(q_ptr), document(nullptr), maxStates(100), autoThumbnailEnabled(false) {}

    UndoVisualization* q;
    QPointer<Document> document; // Use QPointer for safety
    QUndoStack* qtUndoStack; // Weak pointer, owned by UndoStack singleton
    mutable QMutex mutex; // Protect access to treeNodes map
    QHash<quintptr, UndoStateNode> treeNodes; // ID -> Node
    quintptr rootNodeId; // ID of the root node
    quintptr currentId;  // ID of the current state node
    int maxStates;
    bool autoThumbnailEnabled;
    quintptr nextId; // For generating unique node IDs

    // Helper to get current node from UndoStack index
    quintptr getCurrentStateIdFromStack() const {
        if (!qtUndoStack) return 0;
        // This is a simplification. UndoStack doesn't directly expose state IDs.
        // A real implementation would need to track IDs alongside QUndoCommand pushes.
        // For now, we'll use the index as a proxy, though it's not ideal for branching.
        // A better approach is to listen to push/undo/redo and maintain an internal map.
        // This requires deeper integration with UndoStack's command execution.
        // Let's assume an internal map: QHash<QUndoCommand*, quintptr> commandToId;
        // This is complex and requires modifying UndoStack. We'll outline the concept.
        // For this stub, we'll use a simple counter and assume linear history.
        // This does NOT support branching correctly.
        return currentId; // Return the last known current ID
    }

    // Helper to build the tree structure from UndoStack's linear history
    // This is a simplified approach assuming no branching for now.
    void rebuildTreeFromLinearHistory() {
        if (!qtUndoStack) return;

        QMutexLocker locker(&mutex);
        treeNodes.clear();
        rootNodeId = 0;
        currentId = 0;

        // This logic is challenging without deep UndoStack integration.
        // We'll simulate a simple linear tree based on UndoStack's state.
        // A real branching tree needs UndoStack to track states as nodes in a graph.
        // This requires significant changes to UndoStack or a custom implementation.
        // For now, we'll create a linear chain of states.
        int index = 0; // Start from 0 (initial state)
        int count = qtUndoStack->count();
        int currentIndex = qtUndoStack->index(); // Current position

        for (int i = 0; i <= count; ++i) { // count+1 because index can be at 'count' (after last command)
            quintptr id = nextId++;
            UndoStateNode node;
            node.id = id;
            node.parentId = (i > 0) ? (id - 1) : 0; // Simple linear parent
            node.depth = i;
            node.isCurrent = (i == currentIndex);
            node.timestamp = QDateTime::currentDateTime();
            if (i < count) { // If it's a command state (not the initial state)
                 node.commandText = qtUndoStack->text(i);
                 // Link child to next state
                 if (i + 1 <= count) {
                     node.childIds.append(id + 1);
                 }
            } else {
                 node.commandText = "Initial State";
            }

            treeNodes.insert(id, node);
            if (node.isCurrent) {
                currentId = id;
            }
            if (rootNodeId == 0) { // First node is root
                rootNodeId = id;
            }
        }
        LOG_DEBUG("Rebuilt undo tree with " << treeNodes.size() << " nodes.");
    }
};

// Static instance pointer
UndoVisualization* UndoVisualization::s_instance = nullptr;

UndoVisualization& UndoVisualization::instance()
{
    if (!s_instance) {
        s_instance = new UndoVisualization();
    }
    return *s_instance;
}

UndoVisualization::UndoVisualization(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
    d->nextId = 1; // Start IDs from 1
    // Connect to UndoStack signals to react to changes
    // This requires UndoStack to emit signals when commands are pushed/undone/redone.
    // We'll assume UndoStack::instance() has signals like commandPushed, commandUndone, commandRedone.
    // UndoStack* uStack = &UndoStack::instance();
    // connect(uStack, &UndoStack::indexChanged, this, &UndoVisualization::onUndoStackIndexChanged);
    // connect(uStack, &UndoStack::commandExecuted, this, &UndoVisualization::onUndoCommandExecuted); // Custom signal needed
}

UndoVisualization::~UndoVisualization()
{
    // d->treeNodes will be cleared automatically by QHash destructor
}

void UndoVisualization::setDocument(Document* doc)
{
    if (d->document == doc) return;

    // Disconnect from old document's undo stack if necessary
    if (d->document && d->qtUndoStack) {
         // Disconnect signals from previous UndoStack
         // disconnect(d->qtUndoStack, ...);
    }

    d->document = doc;
    if (doc) {
        UndoStack* uStack = &UndoStack::instance();
        uStack->setDocument(doc); // Ensure UndoStack knows about the document
        d->qtUndoStack = uStack->qsettings(); // Assuming a method to access internal QUndoStack
        // Connect to new document's undo stack signals
        // connect(d->qtUndoStack, ...);
        d->rebuildTreeFromLinearHistory(); // Rebuild for the new document
    } else {
        d->qtUndoStack = nullptr;
        d->clear();
    }
    LOG_DEBUG("UndoVisualization set to document: " << (doc ? doc->filePath() : "nullptr"));
}

Document* UndoVisualization::document() const
{
    return d->document.data(); // Returns nullptr if document was deleted
}

UndoStateNode UndoVisualization::getRootNode() const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->treeNodes.constFind(d->rootNodeId);
    return (it != d->treeNodes.constEnd()) ? it.value() : UndoStateNode();
}

UndoStateNode UndoVisualization::getCurrentNode() const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->treeNodes.constFind(d->currentId);
    return (it != d->treeNodes.constEnd()) ? it.value() : UndoStateNode();
}

UndoStateNode UndoVisualization::getNodeById(quintptr id) const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->treeNodes.constFind(id);
    return (it != d->treeNodes.constEnd()) ? it.value() : UndoStateNode();
}

QList<UndoStateNode> UndoVisualization::getChildren(quintptr parentId) const
{
    QList<UndoStateNode> children;
    QMutexLocker locker(&d->mutex);
    auto parentIt = d->treeNodes.constFind(parentId);
    if (parentIt != d->treeNodes.constEnd()) {
        for (quintptr childId : parentIt->childIds) {
            auto childIt = d->treeNodes.constFind(childId);
            if (childIt != d->treeNodes.constEnd()) {
                children.append(childIt.value());
            }
        }
    }
    return children;
}

QList<UndoStateNode> UndoVisualization::getTreeNodes() const
{
    QMutexLocker locker(&d->mutex);
    QList<UndoStateNode> nodes;
    nodes.reserve(d->treeNodes.size());
    for (const auto& node : d->treeNodes) {
        nodes.append(node);
    }
    return nodes;
}

bool UndoVisualization::navigateToState(quintptr nodeId)
{
    // This requires UndoStack to be able to jump to an arbitrary state ID.
    // UndoStack only knows about command indices. This is a fundamental mismatch.
    // A full implementation requires UndoStack to manage a graph of states, not just a linear stack.
    // For now, we can only navigate to states that correspond to known indices in the current linear history.
    QMutexLocker locker(&d->mutex);
    auto it = d->treeNodes.constFind(nodeId);
    if (it != d->treeNodes.constEnd()) {
        // Find the corresponding index in the linear UndoStack history
        // This is difficult without storing the index in the node itself during rebuildTreeFromLinearHistory.
        // We'd need to map nodeId -> UndoStack index.
        // This requires UndoStack to notify us of the *new* index whenever a command is undone/redone/pushed.
        // This is a complex synchronization problem.
        LOG_WARN("navigateToState: Full implementation requires UndoStack graph support. Attempting linear navigation.");
        // A stub: if nodeId corresponds to a command index (or initial state), navigate.
        // This only works for linear history.
        int targetIndex = static_cast<int>(nodeId - 1); // Assuming linear ID == index + 1
        if (targetIndex >= 0 && targetIndex <= d->qtUndoStack->count()) {
             d->qtUndoStack->setIndex(targetIndex);
             // onUndoStackIndexChanged will be called, updating the tree
             return true;
        }
    }
    return false;
}

bool UndoVisualization::annotateState(quintptr nodeId, const QString& annotation)
{
    QMutexLocker locker(&d->mutex);
    auto it = d->treeNodes.find(nodeId);
    if (it != d->treeNodes.end()) {
        it->annotation = annotation;
        emit annotationChanged(nodeId, annotation);
        return true;
    }
    return false;
}

QString UndoVisualization::annotationForState(quintptr nodeId) const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->treeNodes.constFind(nodeId);
    return (it != d->treeNodes.constEnd()) ? it->annotation : QString();
}

QImage UndoVisualization::generateThumbnailForState(quintptr nodeId)
{
    QMutexLocker locker(&d->mutex);
    auto it = d->treeNodes.find(nodeId);
    if (it == d->treeNodes.end()) return QImage();

    // This is a major challenge. To get a thumbnail of a *past* state,
    // you need to reconstruct the document as it was at that point.
    // This requires either:
    // 1. Storing document snapshots (memory intensive).
    // 2. Replaying all commands up to that state (CPU intensive, potentially unsafe).
    // 3. Having a sophisticated state management system in UndoStack that can diff/restore states.
    // This is beyond the scope of a simple stub. We'll return a placeholder.
    LOG_WARN("generateThumbnailForState: Full implementation requires document state restoration. Returning placeholder.");
    QImage placeholder(100, 140, QImage::Format_RGB32); // Common aspect ratio hint
    placeholder.fill(Qt::lightGray);
    // Draw a simple icon or text indicating "State X"
    // QPainter painter(&placeholder);
    // painter.drawText(placeholder.rect(), Qt::AlignCenter, QString("State\n%1").arg(nodeId));
    it->thumbnail = placeholder;
    emit thumbnailGenerated(nodeId, placeholder);
    return placeholder;
}

int UndoVisualization::maxVisualizedStates() const
{
    QMutexLocker locker(&d->mutex);
    return d->maxStates;
}

void UndoVisualization::setMaxVisualizedStates(int count)
{
    if (count < 1) return;
    QMutexLocker locker(&d->mutex);
    if (d->maxStates != count) {
        d->maxStates = count;
        // Truncate tree if necessary
        // This requires a more complex tree management strategy.
        LOG_INFO("Max visualized undo states set to " << count);
    }
}

void UndoVisualization::setAutoThumbnailGenerationEnabled(bool enabled)
{
    QMutexLocker locker(&d->mutex);
    d->autoThumbnailEnabled = enabled;
    LOG_INFO("Auto-thumbnail generation for undo states is " << (enabled ? "enabled" : "disabled"));
}

bool UndoVisualization::isAutoThumbnailGenerationEnabled() const
{
    QMutexLocker locker(&d->mutex);
    return d->autoThumbnailEnabled;
}

void UndoVisualization::clear()
{
    QMutexLocker locker(&d->mutex);
    d->treeNodes.clear();
    d->rootNodeId = 0;
    d->currentId = 0;
    LOG_DEBUG("Cleared undo visualization tree.");
}

void UndoVisualization::onUndoStackIndexChanged()
{
    // Update the current state ID based on the stack's new index
    // This requires UndoStack to provide a way to map its index to our node ID.
    // A simple approach: maintain a map of UndoStack index -> Node ID inside this class.
    // When UndoStack index changes, look up the corresponding Node ID.
    // This map must be updated whenever commands are pushed/undone/redone.
    // This is part of the complex synchronization needed.
    // For now, rebuild the tree (inefficient but functional for linear history).
    d->rebuildTreeFromLinearHistory();
    emit currentStateChanged();
    emit treeChanged();
}

void UndoVisualization::onUndoCommandExecuted(const QUndoCommand* cmd)
{
    // A new command was pushed onto the stack.
    // Create a new node and add it to the tree.
    // This requires UndoStack to emit this signal *after* the command is added and the index is updated.
    // We need the new index to create the node correctly.
    Q_UNUSED(cmd);
    d->rebuildTreeFromLinearHistory(); // For now, rebuild
    emit treeChanged();
    if (d->autoThumbnailEnabled) {
        // Request thumbnail generation for the new current state
        // This should ideally happen on a background thread.
        UndoStateNode currentNode = getCurrentNode();
        if (currentNode.id != 0) {
            // generateThumbnailForState(currentNode.id); // This might block UI, better to use ThreadPool
        }
    }
}

} // namespace QuantilyxDoc