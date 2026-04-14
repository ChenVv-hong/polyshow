#pragma once

#include <QGraphicsView>
#include <QPoint>
#include <QPointF>

namespace PolyShow
{

/// Displays geometry content with zoom, pan, and mouse feedback.
class GeometryViewer final : public QGraphicsView
{
    Q_OBJECT

public:
    /// Creates the viewer widget.
    explicit GeometryViewer(QWidget *parent = nullptr);

public slots:
    /// Zooms the current view in.
    void zoomIn();

    /// Zooms the current view out.
    void zoomOut();

    /// Fits the visible scene items into the viewport.
    void fitScene();

    /// Resets the view transform to the default state.
    void resetViewTransform();

signals:
    /// Emitted when the mouse moves and a scene position is available.
    void mousePositionChanged(const QPointF &scenePosition);

protected:
    /// Handles wheel-based zoom.
    void wheelEvent(QWheelEvent *event) override;

    /// Starts middle-button panning.
    void mousePressEvent(QMouseEvent *event) override;

    /// Updates mouse coordinates and pans the view when needed.
    void mouseMoveEvent(QMouseEvent *event) override;

    /// Stops middle-button panning.
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    /// Applies a view scale factor.
    void applyZoom(qreal factor);

    /// Updates the cursor for the current drag state.
    void updateDragCursor(bool active);

    bool m_is_panning {false};
    QPoint m_last_mouse_position;
};

} // namespace PolyShow
