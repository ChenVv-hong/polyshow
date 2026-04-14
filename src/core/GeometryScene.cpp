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

struct LayerVisibilityMask
{
    QVector<bool> points;
    QVector<bool> polylines;
    QVector<bool> polygons;
};

/// Builds per-primitive visibility lookup tables for one layer.
LayerVisibilityMask buildLayerVisibilityMask(const PolyShow::LayerData &layer)
{
    LayerVisibilityMask mask;
    mask.points = QVector<bool>(layer.geometry.points.size(), true);
    mask.polylines = QVector<bool>(layer.geometry.polylines.size(), true);
    mask.polygons = QVector<bool>(layer.geometry.polygons.size(), true);

    for (const PolyShow::LayerPrimitiveData &primitive : std::as_const(layer.primitives))
    {
        switch (primitive.reference.kind)
        {
        case PolyShow::PrimitiveKind::Point:
            if (primitive.reference.index >= 0 && primitive.reference.index < mask.points.size())
            {
                mask.points[primitive.reference.index] = primitive.visible;
            }
            break;
        case PolyShow::PrimitiveKind::Polyline:
            if (primitive.reference.index >= 0 && primitive.reference.index < mask.polylines.size())
            {
                mask.polylines[primitive.reference.index] = primitive.visible;
            }
            break;
        case PolyShow::PrimitiveKind::Polygon:
            if (primitive.reference.index >= 0 && primitive.reference.index < mask.polygons.size())
            {
                mask.polygons[primitive.reference.index] = primitive.visible;
            }
            break;
        }
    }

    return mask;
}

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

/// Replaces the stored document data and refreshes the scene.
void GeometryScene::setDocumentData(const DocumentData &documentData)
{
    m_document_data = documentData;
    updateVisibleCounts();
    rebuildScene();
    emit geometryChanged(m_point_count, m_polyline_count, m_polygon_count);
}

/// Clears all stored document data and refreshes the scene.
void GeometryScene::clearDocument()
{
    m_document_data = DocumentData {};
    updateVisibleCounts();
    rebuildScene();
    emit geometryChanged(m_point_count, m_polyline_count, m_polygon_count);
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
    return m_point_count;
}

/// Returns the polyline count.
int GeometryScene::polylineCount() const
{
    return m_polyline_count;
}

/// Returns the polygon count.
int GeometryScene::polygonCount() const
{
    return m_polygon_count;
}

/// Recomputes visible counts from all currently enabled layers and primitives.
void GeometryScene::updateVisibleCounts()
{
    m_point_count = 0;
    m_polyline_count = 0;
    m_polygon_count = 0;

    for (const LayerData &layer : std::as_const(m_document_data.layers))
    {
        if (!layer.visible)
        {
            continue;
        }

        const LayerVisibilityMask mask = buildLayerVisibilityMask(layer);
        for (const bool isVisible : std::as_const(mask.points))
        {
            if (isVisible)
            {
                ++m_point_count;
            }
        }

        for (const bool isVisible : std::as_const(mask.polylines))
        {
            if (isVisible)
            {
                ++m_polyline_count;
            }
        }

        for (const bool isVisible : std::as_const(mask.polygons))
        {
            if (isVisible)
            {
                ++m_polygon_count;
            }
        }
    }
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

    for (const LayerData &layer : std::as_const(m_document_data.layers))
    {
        if (!layer.visible)
        {
            continue;
        }

        const LayerVisibilityMask mask = buildLayerVisibilityMask(layer);

        if (m_render_mode != RenderMode::Points)
        {
            // Draw polygons first so subsequent lines and points naturally appear above them.
            for (int i = 0; i < layer.geometry.polygons.size(); ++i)
            {
                if (!mask.polygons.value(i))
                {
                    continue;
                }

                const Polygon2D &polygon = layer.geometry.polygons.at(i);
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
            for (int i = 0; i < layer.geometry.polylines.size(); ++i)
            {
                if (!mask.polylines.value(i))
                {
                    continue;
                }

                const Polyline2D &polyline = layer.geometry.polylines.at(i);
                if (polyline.vertices.size() < 2)
                {
                    continue;
                }

                // Build one painter path so the entire polyline can be submitted in a single scene item.
                QPainterPath path(polyline.vertices.first().toPointF());
                for (int vertexIndex = 1; vertexIndex < polyline.vertices.size(); ++vertexIndex)
                {
                    path.lineTo(polyline.vertices.at(vertexIndex).toPointF());
                }

                QPen polylinePen(polyline.style.color);
                polylinePen.setWidthF(polyline.style.width);
                addPath(path, polylinePen);
            }
        }

        // Standalone points are visible in every render mode.
        for (int i = 0; i < layer.geometry.points.size(); ++i)
        {
            if (!mask.points.value(i))
            {
                continue;
            }

            const PointShape2D &point = layer.geometry.points.at(i);
            addPointMarker(point.point, point.style);
        }

        if (m_render_mode == RenderMode::Points)
        {
            // In point mode, expose polyline and polygon vertices as standalone point markers.
            for (int i = 0; i < layer.geometry.polylines.size(); ++i)
            {
                if (!mask.polylines.value(i))
                {
                    continue;
                }

                const Polyline2D &polyline = layer.geometry.polylines.at(i);
                for (const Point2D &vertex : polyline.vertices)
                {
                    addPointMarker(vertex, polyline.style);
                }
            }

            for (int i = 0; i < layer.geometry.polygons.size(); ++i)
            {
                if (!mask.polygons.value(i))
                {
                    continue;
                }

                const Polygon2D &polygon = layer.geometry.polygons.at(i);
                for (const Point2D &vertex : polygon.vertices)
                {
                    addPointMarker(vertex, polygon.style);
                }
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
