#pragma once

#include <QGraphicsView>
#include <QPoint>
#include <QPointF>

class QEvent;

namespace PolyShow
{

/// Displays geometry content with zoom, pan, and mouse feedback.
class GeometryViewer final : public QGraphicsView
{
    Q_OBJECT

public:
    /// Supported interaction tools for the workspace.
    enum class ToolMode
    {
        Browse,
        DrawPoint,
        DrawPolyline,
        DrawPolygon
    };
    Q_ENUM(ToolMode)

    /// Creates the viewer widget.
    explicit GeometryViewer(QWidget *parent = nullptr);

    /// Switches the active workspace tool.
    void setToolMode(ToolMode toolMode);

    /// Returns the active workspace tool.
    [[nodiscard]]
    ToolMode toolMode() const;

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

    /// Emitted when the mouse leaves the workspace viewport.
    void workspaceHoverExited();

    /// Emitted when the user clicks one primitive in the scene.
    void primitiveActivated(int layerIndex, int primitiveIndex, bool toggleRequested);

    /// Emitted when the user clicks empty scene space.
    void emptyAreaActivated(bool toggleRequested);

    /// Emitted when the active drawing tool receives one vertex click.
    void drawingPointRequested(const QPointF &scenePosition);

    /// Emitted when the active drawing tool requests a commit.
    void drawingFinishedRequested();

protected:
    /// Draws the viewport-relative dynamic grid background.
    void drawBackground(QPainter *painter, const QRectF &rect) override;

    /// Draws the screen-space scale overlay.
    void drawForeground(QPainter *painter, const QRectF &rect) override;

    /// Handles wheel-based zoom.
    void wheelEvent(QWheelEvent *event) override;

    /// Starts middle-button panning.
    void mousePressEvent(QMouseEvent *event) override;

    /// Updates mouse coordinates and pans the view when needed.
    void mouseMoveEvent(QMouseEvent *event) override;

    /// Stops middle-button panning.
    void mouseReleaseEvent(QMouseEvent *event) override;

    /// Clears hover-only state when the mouse leaves the viewport.
    void leaveEvent(QEvent *event) override;

private:
    /// Returns whether one selectable primitive is under the viewport position.
    [[nodiscard]]
    bool hasSelectablePrimitiveAt(const QPoint &viewPosition) const;

    /// Updates the workspace cursor for the current interaction state.
    void updateInteractionCursor(const QPoint &viewPosition);

    /// Applies a view scale factor.
    void applyZoom(qreal factor);

    /// Updates the cursor for the current drag state.
    void updateDragCursor(bool active);

    ToolMode m_tool_mode {ToolMode::Browse};
    bool m_is_panning {false};
    QPoint m_last_mouse_position;
};

} // namespace PolyShow
