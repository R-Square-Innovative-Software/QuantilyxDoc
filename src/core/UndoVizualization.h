/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef QUANTILYX_UNDOVISUALIZATION_H
#define QUANTILYX_UNDOVISUALIZATION_H

#include <QObject>
#include <QDateTime>
#include <QImage>
#include <QUndoCommand>
#include <memory>
#include <functional>

namespace QuantilyxDoc {

class Document;
class UndoStack;

/**
 * @brief Represents a single state in the undo/redo history tree.
 * 
 * Each node contains information about the command that created this state,
 * a thumbnail of the document at this state (if applicable), and links
 * to parent and child states, allowing for branching history.
 */
struct UndoStateNode {
    quintptr id;                    // Unique identifier for this state
    QString commandText;            // Text description from the command
    QDateTime timestamp;            // When this state was created
    QImage thumbnail;               // Thumbnail of the document at this state (optional)
    quintptr parentId;              // ID of the parent state (0 if root)
    QList<quintptr> childIds;       // IDs of child states (possible branches)
    int depth;                      // Depth in the tree (for layout)
    bool isCurrent;                 // Is this the current active state?
    bool isSaved;                   // Was this state saved to disk?
    QString annotation;             // User-provided annotation for this state
    QVariantMap metadata;           // Additional metadata associated with the state

    UndoStateNode() : id(0), parentId(0), depth(0), isCurrent(false), isSaved(false) {}
};

/**
 * @brief Provides data and logic for visualizing the undo/redo history tree.
 * 
 * This class interfaces with the UndoStack to build and maintain a tree
 * representation of the document's modification history, suitable for
 * display in a UI panel. It can generate thumbnails and manage annotations.
 */
class UndoVisualization : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param parent Parent object.
     */
    explicit UndoVisualization(QObject* parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~UndoVisualization() override;

    /**
     * @brief Get singleton instance.
     * @return Reference to the global UndoVisualization instance.
     */
    static UndoVisualization& instance();

    /**
     * @brief Associate this visualization with a specific document's undo stack.
     * @param doc The document to visualize.
     */
    void setDocument(Document* doc);

    /**
     * @brief Get the currently associated document.
     * @return Pointer to the document, or nullptr.
     */
    Document* document() const;

    /**
     * @brief Get the root node of the undo tree.
     * @return Root node, or an invalid node if no document is set or tree is empty.
     */
    UndoStateNode getRootNode() const;

    /**
     * @brief Get the current state node (the one the document is currently at).
     * @return Current state node, or an invalid node if none.
     */
    UndoStateNode getCurrentNode() const;

    /**
     * @brief Get a specific node by its ID.
     * @param id The node ID.
     * @return The node, or an invalid node if not found.
     */
    UndoStateNode getNodeById(quintptr id) const;

    /**
     * @brief Get all child nodes for a given parent node ID.
     * @param parentId The ID of the parent node.
     * @return List of child nodes.
     */
    QList<UndoStateNode> getChildren(quintptr parentId) const;

    /**
     * @brief Get the full tree structure as a list of nodes.
     * @return List of all nodes in the tree.
     */
    QList<UndoStateNode> getTreeNodes() const;

    /**
     * @brief Navigate the document's undo stack to a specific state.
     * @param nodeId The ID of the target state node.
     * @return True if navigation was successful.
     */
    bool navigateToState(quintptr nodeId);

    /**
     * @brief Annotate a specific state node with a user note.
     * @param nodeId The ID of the node to annotate.
     * @param annotation The annotation text.
     * @return True if annotation was set.
     */
    bool annotateState(quintptr nodeId, const QString& annotation);

    /**
     * @brief Get the annotation for a specific state node.
     * @param nodeId The ID of the node.
     * @return The annotation text, or an empty string.
     */
    QString annotationForState(quintptr nodeId) const;

    /**
     * @brief Generate a thumbnail for a specific state node.
     * This might involve temporarily applying commands up to that state.
     * @param nodeId The ID of the node to generate a thumbnail for.
     * @return The generated thumbnail image, or a null image.
     */
    QImage generateThumbnailForState(quintptr nodeId);

    /**
     * @brief Get the maximum number of states to visualize before truncating.
     * @return Maximum state count.
     */
    int maxVisualizedStates() const;

    /**
     * @brief Set the maximum number of states to visualize before truncating.
     * @param count New maximum count.
     */
    void setMaxVisualizedStates(int count);

    /**
     * @brief Enable or disable automatic thumbnail generation for new states.
     * @param enabled True to enable.
     */
    void setAutoThumbnailGenerationEnabled(bool enabled);

    /**
     * @brief Check if automatic thumbnail generation is enabled.
     * @return True if enabled.
     */
    bool isAutoThumbnailGenerationEnabled() const;

    /**
     * @brief Clear the visualization data for the current document.
     */
    void clear();

signals:
    /**
     * @brief Emitted when the visualization tree structure changes.
     */
    void treeChanged();

    /**
     * @brief Emitted when the current state in the tree changes.
     */
    void currentStateChanged();

    /**
     * @brief Emitted when a thumbnail is generated for a state.
     * @param nodeId The ID of the state.
     * @param thumbnail The generated thumbnail.
     */
    void thumbnailGenerated(quintptr nodeId, const QImage& thumbnail);

    /**
     * @brief Emitted when an annotation is added or modified.
     * @param nodeId The ID of the state.
     * @param annotation The new annotation.
     */
    void annotationChanged(quintptr nodeId, const QString& annotation);

private slots:
    void onUndoStackIndexChanged(); // React to UndoStack changes
    void onUndoCommandExecuted(const QUndoCommand* cmd); // Capture new states

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace QuantilyxDoc

#endif // QUANTILYX_UNDOVISUALIZATION_H