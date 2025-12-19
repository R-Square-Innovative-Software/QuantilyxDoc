/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "UndoStack.h"
#include "Document.h"
#include <QUndoStack>
#include <QUndoCommand>
#include <QPointer>
#include <QDebug>

namespace QuantilyxDoc {

class UndoStack::Private {
public:
    Private() : q(nullptr), qtUndoStack(nullptr), document(nullptr) {}
    UndoStack* q;
    QUndoStack* qtUndoStack;
    QPointer<Document> document; // Use QPointer to handle potential Document destruction
};

// Static instance pointer
UndoStack* UndoStack::s_instance = nullptr;

UndoStack& UndoStack::instance()
{
    if (!s_instance) {
        s_instance = new UndoStack();
    }
    return *s_instance;
}

UndoStack::UndoStack(QObject* parent)
    : QObject(parent)
    , d(new Private())
{
    d->q = this;
    d->qtUndoStack = new QUndoStack(this);

    // Connect Qt's signals to ours
    connect(d->qtUndoStack, &QUndoStack::canUndoChanged,
            this, &UndoStack::canUndoChanged);
    connect(d->qtUndoStack, &QUndoStack::canRedoChanged,
            this, &UndoStack::canRedoChanged);
    connect(d->qtUndoStack, &QUndoStack::undoTextChanged,
            this, &UndoStack::undoTextChanged);
    connect(d->qtUndoStack, &QUndoStack::redoTextChanged,
            this, &UndoStack::redoTextChanged);
    connect(d->qtUndoStack, &QUndoStack::cleanChanged,
            this, &UndoStack::cleanChanged);
}

UndoStack::~UndoStack()
{
    // d->qtUndoStack is automatically deleted by QObject parent-child mechanism
}

void UndoStack::push(QUndoCommand* cmd)
{
    if (d->qtUndoStack) {
        d->qtUndoStack->push(cmd);
    }
}

void UndoStack::undo()
{
    if (d->qtUndoStack) {
        d->qtUndoStack->undo();
    }
}

void UndoStack::redo()
{
    if (d->qtUndoStack) {
        d->qtUndoStack->redo();
    }
}

void UndoStack::clear()
{
    if (d->qtUndoStack) {
        d->qtUndoStack->clear();
    }
}

bool UndoStack::canUndo() const
{
    return d->qtUndoStack ? d->qtUndoStack->canUndo() : false;
}

bool UndoStack::canRedo() const
{
    return d->qtUndoStack ? d->qtUndoStack->canRedo() : false;
}

QString UndoStack::undoText() const
{
    return d->qtUndoStack ? d->qtUndoStack->undoText() : QString();
}

QString UndoStack::redoText() const
{
    return d->qtUndoStack ? d->qtUndoStack->redoText() : QString();
}

int UndoStack::undoStackSize() const
{
    return d->qtUndoStack ? d->qtUndoStack->count() - d->qtUndoStack->index() : 0;
}

int UndoStack::redoStackSize() const
{
    return d->qtUndoStack ? d->qtUndoStack->index() : 0;
}

void UndoStack::setUndoLimit(int limit)
{
    if (d->qtUndoStack) {
        d->qtUndoStack->setUndoLimit(limit);
    }
}

int UndoStack::undoLimit() const
{
    return d->qtUndoStack ? d->qtUndoStack->undoLimit() : 0;
}

void UndoStack::beginMacro(const QString& text)
{
    if (d->qtUndoStack) {
        d->qtUndoStack->beginMacro(text);
    }
}

void UndoStack::endMacro()
{
    if (d->qtUndoStack) {
        d->qtUndoStack->endMacro();
    }
}

Document* UndoStack::document() const
{
    return d->document.data(); // Returns nullptr if document was deleted
}

void UndoStack::setDocument(Document* doc)
{
    d->document = doc; // QPointer handles deletion automatically
}

} // namespace QuantilyxDoc