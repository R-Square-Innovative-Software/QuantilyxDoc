/**
 * QuantilyxDoc - Professional Document Editor
 * Copyright (C) 2025 R² Innovative Software
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "Page.h"
#include "Document.h"
#include "../annotations/Annotation.h"
#include <QRectF>
#include <QPointF>
#include <QVariantMap>

namespace QuantilyxDoc {

class Page::Private {
public:
    Private()
        : pageNumber(0)
        , pageIndex(0)
        , rotation(PageRotation::Degrees0)
        , visible(true)
    {}
    int pageNumber;
    int pageIndex;
    QSizeF size;
    PageRotation rotation;
    QString label;
    QString title;
    bool visible;
    QRectF contentBox;
    QList<Annotation*> annotations;
    PageContent* content;
    PageRenderer* renderer;
};

Page::Page(Document* document, QObject* parent)
    : QObject(parent)
    , d(new Private())
    , m_document(document)
{
    d->content = nullptr;
    d->renderer = nullptr;
}

Page::~Page()
{
    qDeleteAll(d->annotations);
}

int Page::pageNumber() const
{
    return d->pageNumber;
}

int Page::pageIndex() const
{
    return d->pageIndex;
}

QSizeF Page::size() const
{
    // Apply rotation to size if needed
    if (d->rotation == PageRotation::Degrees90 || d->rotation == PageRotation::Degrees270) {
        return QSizeF(d->size.height(), d->size.width());
    }
    return d->size;
}

PageRotation Page::rotation() const
{
    return d->rotation;
}

QString Page::label() const
{
    return d->label;
}

QString Page::title() const
{
    return d->title;
}

bool Page::isVisible() const
{
    return d->visible;
}

QRectF Page::contentBox() const
{
    return d->contentBox;
}

QList<Annotation*> Page::annotations() const
{
    return d->annotations;
}

void Page::addAnnotation(Annotation* annotation)
{
    if (!d->annotations.contains(annotation)) {
        d->annotations.append(annotation);
        annotation->setParent(this);
        emit annotationsChanged();
    }
}

void Page::removeAnnotation(Annotation* annotation)
{
    if (d->annotations.removeOne(annotation)) {
        annotation->setParent(nullptr);
        emit annotationsChanged();
    }
}

PageContent* Page::content() const
{
    return d->content;
}

PageRenderer* Page::renderer() const
{
    return d->renderer;
}

QString Page::text() const
{
    return QString();
}

QList<QRectF> Page::searchText(const QString& text, bool caseSensitive, bool wholeWords) const
{
    Q_UNUSED(text);
    Q_UNUSED(caseSensitive);
    Q_UNUSED(wholeWords);
    return QList<QRectF>();
}

QObject* Page::hitTest(const QPointF& position) const
{
    Q_UNUSED(position);
    return nullptr;
}

QList<QObject*> Page::links() const
{
    return QList<QObject*>();
}

QVariantMap Page::metadata() const
{
    return QVariantMap();
}

void Page::setPageNumber(int number)
{
    d->pageNumber = number;
    d->pageIndex = number - 1; // Convert to 0-based index
}

void Page::setSize(const QSizeF& size)
{
    d->size = size;
}

void Page::setRotation(PageRotation rotation)
{
    d->rotation = rotation;
}

void Page::setLabel(const QString& label)
{
    d->label = label;
}

void Page::setTitle(const QString& title)
{
    d->title = title;
}

void Page::setVisible(bool visible)
{
    d->visible = visible;
}

void Page::setContentBox(const QRectF& box)
{
    d->contentBox = box;
}

} // namespace QuantilyxDoc