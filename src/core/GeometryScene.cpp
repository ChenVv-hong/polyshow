#include "core/GeometryScene.h"

#include <QBrush>
#include <QColor>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QRectF>

#include <utility>

namespace
{

// Keep a large default grid so small and medium scenes still have context.
constexpr int kGridHalfSize = 2000;
constexpr int kGridStep = 50;

} // namespace

namespace PolyShow
{

/// Creates the scene and initializes the default visible range.
GeometryScene::GeometryScene(QObject *parent)
    : QGraphicsScene(parent)
{
    // Set the scene rect up front so the viewer always has a coordinate reference.
    setSceneRect(-kGridHalfSize, -kGridHalfSize, kGridHalfSize * 2, kGridHalfSize * 2);
    rebuildScene();
}

/// Replaces the stored geometry data and refreshes the scene.
void GeometryScene::setGeometryData(const GeometryData &geometryData)
{
    m_geometry_data = geometryData;
    rebuildScene();
    emit geometryChanged(pointCount(), polylineCount(), polygonCount());
}

/// Clears all stored geometry data and refreshes the scene.
void GeometryScene::clearGeometry()
{
    m_geometry_data = GeometryData {};
    rebuildScene();
    emit geometryChanged(0, 0, 0);
}

/// Switches the render mode when the requested mode changes.
void GeometryScene::setRenderMode(RenderMode renderMode)
{
    if (m_render_mode == renderMode)
    {
        return;
    }

    m_render_mode = renderMode;
    rebuildScene();
}

/// Returns the current render mode.
GeometryScene::RenderMode GeometryScene::renderMode() const
{
    return m_render_mode;
}

/// Shows or hides the background grid.
void GeometryScene::setGridVisible(bool visible)
{
    if (m_is_grid_visible == visible)
    {
        return;
    }

    m_is_grid_visible = visible;
    rebuildScene();
}

/// Returns whether the grid is currently visible.
bool GeometryScene::isGridVisible() const
{
    return m_is_grid_visible;
}

/// Returns the standalone point count.
int GeometryScene::pointCount() const
{
    return m_geometry_data.points.size();
}

/// Returns the polyline count.
int GeometryScene::polylineCount() const
{
    return m_geometry_data.polylines.size();
}

/// Returns the polygon count.
int GeometryScene::polygonCount() const
{
    return m_geometry_data.polygons.size();
}

/// Rebuilds every scene item from the current stored state.
void GeometryScene::rebuildScene()
{
    // Rebuild everything from scratch so render mode switches do not leave stale items behind.
    clear();
    rebuildGrid();

    const auto addPointMarker = [this](const Point2D &point, const PrimitiveStyle &style) {
        // Draw every point as a filled circle so point mode and vertex previews look identical.
        const double radius = style.point_size;
        const QRectF markerRect(point.x - radius, point.y - radius, radius * 2.0, radius * 2.0);
        QPen pointPen(style.color);
        pointPen.setWidthF(1.0);
        addEllipse(markerRect, pointPen, QBrush(style.color));
    };

    if (m_render_mode != RenderMode::Points)
    {
        // Draw polygons first so subsequent lines and points naturally appear above them.
        for (const Polygon2D &polygon : std::as_const(m_geometry_data.polygons))
        {
            if (polygon.vertices.size() < 3)
            {
                continue;
            }

            // Convert the project vertex format to the Qt polygon type expected by the scene.
            QPolygonF qtPolygon;
            qtPolygon.reserve(polygon.vertices.size());
            for (const Point2D &vertex : polygon.vertices)
            {
                qtPolygon << vertex.toPointF();
            }

            QPen polygonPen(polygon.style.color);
            polygonPen.setWidthF(polygon.style.width);
            const bool shouldUseFill = m_render_mode == RenderMode::Solid && polygon.style.fill_enabled;
            const QBrush polygonBrush = shouldUseFill ? QBrush(polygon.style.fill_color) : QBrush(Qt::NoBrush);
            addPolygon(qtPolygon, polygonPen, polygonBrush);
        }

        // Draw polylines after polygons so wireframe details stay crisp.
        for (const Polyline2D &polyline : std::as_const(m_geometry_data.polylines))
        {
            if (polyline.vertices.size() < 2)
            {
                continue;
            }

            // Build one painter path so the entire polyline can be submitted in a single scene item.
            QPainterPath path(polyline.vertices.first().toPointF());
            for (int i = 1; i < polyline.vertices.size(); ++i)
            {
                path.lineTo(polyline.vertices[i].toPointF());
            }

            QPen polylinePen(polyline.style.color);
            polylinePen.setWidthF(polyline.style.width);
            addPath(path, polylinePen);
        }
    }

    // Standalone points are visible in every render mode.
    for (const PointShape2D &point : std::as_const(m_geometry_data.points))
    {
        addPointMarker(point.point, point.style);
    }

    if (m_render_mode == RenderMode::Points)
    {
        // In point mode, expose polyline and polygon vertices as standalone point markers.
        for (const Polyline2D &polyline : std::as_const(m_geometry_data.polylines))
        {
            for (const Point2D &vertex : polyline.vertices)
            {
                addPointMarker(vertex, polyline.style);
            }
        }

        for (const Polygon2D &polygon : std::as_const(m_geometry_data.polygons))
        {
            for (const Point2D &vertex : polygon.vertices)
            {
                addPointMarker(vertex, polygon.style);
            }
        }
    }
}

/// Rebuilds the background grid and axis lines.
void GeometryScene::rebuildGrid()
{
    if (!m_is_grid_visible)
    {
        return;
    }

    // Use a light grid so the geometry remains the visual focus.
    QPen gridPen(QColor(224, 228, 235));
    gridPen.setWidthF(0.0);

    // Use a slightly darker pen for the origin axes.
    QPen axisPen(QColor(180, 180, 180));
    axisPen.setWidthF(0.0);

    for (int x = -kGridHalfSize; x <= kGridHalfSize; x += kGridStep)
    {
        addLine(x, -kGridHalfSize, x, kGridHalfSize, (x == 0) ? axisPen : gridPen);
    }

    for (int y = -kGridHalfSize; y <= kGridHalfSize; y += kGridStep)
    {
        addLine(-kGridHalfSize, y, kGridHalfSize, y, (y == 0) ? axisPen : gridPen);
    }
}

} // namespace PolyShow
