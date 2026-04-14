#include "ui/GeometryViewer.h"

#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

namespace PolyShow
{

/// Creates the viewer and configures the default interaction behavior.
GeometryViewer::GeometryViewer(QWidget *parent)
    : QGraphicsView(parent)
{
    // Enable smoothing so points and lines remain readable while zooming.
    setRenderHint(QPainter::Antialiasing, true);

    // Keep future pixmap-based content looking acceptable when scaled.
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setBackgroundBrush(QColor(248, 250, 252));
}

/// Applies the standard zoom-in ratio.
void GeometryViewer::zoomIn()
{
    applyZoom(1.15);
}

/// Applies the standard zoom-out ratio.
void GeometryViewer::zoomOut()
{
    applyZoom(1.0 / 1.15);
}

/// Fits all scene items into the current viewport.
void GeometryViewer::fitScene()
{
    if (scene() == nullptr)
    {
        return;
    }

    const QRectF itemsRect = scene()->itemsBoundingRect();
    fitInView(itemsRect.isEmpty() ? scene()->sceneRect() : itemsRect, Qt::KeepAspectRatio);
}

/// Resets the transform and recenters the origin.
void GeometryViewer::resetViewTransform()
{
    resetTransform();
    centerOn(0.0, 0.0);
}

/// Handles wheel-based zoom input.
void GeometryViewer::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
    {
        zoomIn();
    }
    else
    {
        zoomOut();
    }

    event->accept();
}

/// Starts panning when the middle mouse button is pressed.
void GeometryViewer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        m_is_panning = true;
        m_last_mouse_position = event->pos();
        updateDragCursor(true);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

/// Updates the mouse position signal and pans while middle-dragging.
void GeometryViewer::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF scenePosition = mapToScene(event->pos());
    emit mousePositionChanged(scenePosition);

    if (m_is_panning)
    {
        const QPoint delta = event->pos() - m_last_mouse_position;
        m_last_mouse_position = event->pos();

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

/// Stops panning when the middle mouse button is released.
void GeometryViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && m_is_panning)
    {
        m_is_panning = false;
        updateDragCursor(false);
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

/// Applies a scale transform to the view.
void GeometryViewer::applyZoom(qreal factor)
{
    scale(factor, factor);
}

/// Chooses the cursor shape for the current drag state.
void GeometryViewer::updateDragCursor(bool active)
{
    setCursor(active ? Qt::ClosedHandCursor : Qt::ArrowCursor);
}

} // namespace PolyShow
