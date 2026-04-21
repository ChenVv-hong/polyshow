#include "core/GeometryScene.h"

#include "core/PrimitiveEditing.h"
#include "style/RenderTheme.h"

#include <QBrush>
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QRectF>

#include <algorithm>
#include <utility>

namespace
{

// Keep a large default grid so small and medium scenes still have context.
constexpr int kGridHalfSize = 2000;
constexpr int kGridStep = 50;
constexpr int kLayerIndexRole = 1;
constexpr int kPrimitiveIndexRole = 2;
constexpr int kSelectionOverlayRole = 3;
constexpr double kSelectionPadding = 6.0;
constexpr double kMinimumSelectionSize = 12.0;

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

PolyShow::SelectionState normalizeSelectionState(
    const PolyShow::DocumentData &documentData, const PolyShow::SelectionState &selectionState)
{
    if (selectionState.kind == PolyShow::SelectionKind::None)
    {
        return {};
    }

    if (selectionState.layer_index < 0 || selectionState.layer_index >= documentData.layers.size())
    {
        return {};
    }

    if (selectionState.kind == PolyShow::SelectionKind::Layer)
    {
        if (!documentData.layers.at(selectionState.layer_index).visible)
        {
            return {};
        }
        return selectionState;
    }

    const PolyShow::LayerData &layer = documentData.layers.at(selectionState.layer_index);
    if (!layer.visible)
    {
        return {};
    }

    if (selectionState.primitive_index < 0 || selectionState.primitive_index >= layer.primitives.size())
    {
        return {};
    }

    if (!layer.primitives.at(selectionState.primitive_index).visible)
    {
        return {};
    }

    return selectionState;
}

int primitiveListIndexForReference(
    const PolyShow::LayerData &layer, PolyShow::PrimitiveKind primitiveKind, int geometryIndex)
{
    for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
    {
        const PolyShow::LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
        if (primitive.reference.kind == primitiveKind && primitive.reference.index == geometryIndex)
        {
            return primitiveIndex;
        }
    }

    return -1;
}

/// Returns the padded selection bounds for one primitive.
QRectF primitiveSelectionBounds(const PolyShow::LayerData &layer, int primitiveIndex)
{
    if (primitiveIndex < 0 || primitiveIndex >= layer.primitives.size())
    {
        return {};
    }

    const PolyShow::LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
    const QVector<PolyShow::Point2D> points = PolyShow::primitivePoints(layer, primitiveIndex);
    if (points.isEmpty())
    {
        return {};
    }

    double minX = points.first().x;
    double maxX = points.first().x;
    double minY = points.first().y;
    double maxY = points.first().y;

    for (const PolyShow::Point2D &point : points)
    {
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }

    const PolyShow::PrimitiveStyle style = PolyShow::primitiveStyle(layer, primitiveIndex);
    const double shapePadding = primitive.reference.kind == PolyShow::PrimitiveKind::Point
        ? std::max(style.point_size, kMinimumSelectionSize * 0.5)
        : std::max(style.width * 0.5, 1.0);
    QRectF bounds(minX, minY, maxX - minX, maxY - minY);
    bounds = bounds.adjusted(
        -(shapePadding + kSelectionPadding),
        -(shapePadding + kSelectionPadding),
        shapePadding + kSelectionPadding,
        shapePadding + kSelectionPadding);

    if (bounds.width() < kMinimumSelectionSize)
    {
        const double delta = (kMinimumSelectionSize - bounds.width()) * 0.5;
        bounds.adjust(-delta, 0.0, delta, 0.0);
    }

    if (bounds.height() < kMinimumSelectionSize)
    {
        const double delta = (kMinimumSelectionSize - bounds.height()) * 0.5;
        bounds.adjust(0.0, -delta, 0.0, delta);
    }

    return bounds;
}

/// Returns the union bounds of every visible primitive in one layer.
QRectF layerSelectionBounds(const PolyShow::LayerData &layer)
{
    QRectF combinedBounds;
    bool hasBounds = false;

    for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
    {
        const PolyShow::LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
        if (!primitive.visible)
        {
            continue;
        }

        const QRectF bounds = primitiveSelectionBounds(layer, primitiveIndex);
        if (bounds.isEmpty() && bounds.isNull())
        {
            continue;
        }

        combinedBounds = hasBounds ? combinedBounds.united(bounds) : bounds;
        hasBounds = true;
    }

    return hasBounds ? combinedBounds : QRectF {};
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
    m_selection_state = normalizeSelectionState(m_document_data, m_selection_state);
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

/// Replaces the active inspector preview suppression state.
void GeometryScene::setEditPreviewState(const PrimitiveEditPreviewState &previewState, bool shouldRebuildScene)
{
    if (m_edit_preview_state == previewState)
    {
        if (shouldRebuildScene)
        {
            this->rebuildScene();
        }
        return;
    }

    m_edit_preview_state = previewState;
    if (shouldRebuildScene)
    {
        this->rebuildScene();
    }
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

SelectionState GeometryScene::selectionState() const
{
    return m_selection_state;
}

void GeometryScene::setSelectionState(const SelectionState &selectionState)
{
    const SelectionState normalizedSelection = normalizeSelectionState(m_document_data, selectionState);
    if (normalizedSelection == m_selection_state)
    {
        return;
    }

    m_selection_state = normalizedSelection;
    rebuildScene();
    emit selectionStateChanged(m_selection_state);
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
    const RenderColors &renderColors = RenderTheme::colors();
    const bool suppressSelectedPrimitive = m_edit_preview_state.hide_selected_primitive
        && m_edit_preview_state.selection_state.kind == SelectionKind::Primitive;

    const auto addPointMarker = [this](
                                    const Point2D &point,
                                    const PrimitiveStyle &style,
                                    int layerIndex,
                                    int primitiveIndex) {
        // Draw every point as a filled circle so point mode and vertex previews look identical.
        const double radius = style.point_size;
        const QRectF markerRect(point.x - radius, point.y - radius, radius * 2.0, radius * 2.0);
        QPen pointPen(style.color);
        pointPen.setWidthF(1.0);
        auto *item = addEllipse(markerRect, pointPen, QBrush(style.color));
        item->setData(kLayerIndexRole, layerIndex);
        item->setData(kPrimitiveIndexRole, primitiveIndex);
    };

    for (int layerIndex = 0; layerIndex < m_document_data.layers.size(); ++layerIndex)
    {
        const LayerData &layer = m_document_data.layers.at(layerIndex);
        if (!layer.visible)
        {
            continue;
        }

        const auto shouldSuppressPrimitive = [this, suppressSelectedPrimitive, layerIndex](int primitiveIndex) {
            return suppressSelectedPrimitive
                && m_edit_preview_state.selection_state.layer_index == layerIndex
                && m_edit_preview_state.selection_state.primitive_index == primitiveIndex;
        };

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
                const int primitiveIndex = primitiveListIndexForReference(layer, PrimitiveKind::Polygon, i);
                if (shouldSuppressPrimitive(primitiveIndex))
                {
                    continue;
                }
                const bool shouldUseFill = m_render_mode == RenderMode::Solid && polygon.style.fill_enabled;
                const QBrush polygonBrush = shouldUseFill ? QBrush(polygon.style.fill_color) : QBrush(Qt::NoBrush);
                auto *item = addPolygon(qtPolygon, polygonPen, polygonBrush);
                item->setData(kLayerIndexRole, layerIndex);
                item->setData(kPrimitiveIndexRole, primitiveIndex);
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
                const int primitiveIndex = primitiveListIndexForReference(layer, PrimitiveKind::Polyline, i);
                if (shouldSuppressPrimitive(primitiveIndex))
                {
                    continue;
                }
                auto *item = addPath(path, polylinePen);
                item->setData(kLayerIndexRole, layerIndex);
                item->setData(kPrimitiveIndexRole, primitiveIndex);
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
            const int primitiveIndex = primitiveListIndexForReference(layer, PrimitiveKind::Point, i);
            if (shouldSuppressPrimitive(primitiveIndex))
            {
                continue;
            }
            addPointMarker(point.point, point.style, layerIndex, primitiveIndex);
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
                const int primitiveIndex = primitiveListIndexForReference(layer, PrimitiveKind::Polyline, i);
                if (shouldSuppressPrimitive(primitiveIndex))
                {
                    continue;
                }
                for (const Point2D &vertex : polyline.vertices)
                {
                    addPointMarker(vertex, polyline.style, layerIndex, primitiveIndex);
                }
            }

            for (int i = 0; i < layer.geometry.polygons.size(); ++i)
            {
                if (!mask.polygons.value(i))
                {
                    continue;
                }

                const Polygon2D &polygon = layer.geometry.polygons.at(i);
                const int primitiveIndex = primitiveListIndexForReference(layer, PrimitiveKind::Polygon, i);
                if (shouldSuppressPrimitive(primitiveIndex))
                {
                    continue;
                }
                for (const Point2D &vertex : polygon.vertices)
                {
                    addPointMarker(vertex, polygon.style, layerIndex, primitiveIndex);
                }
            }
        }
    }

    QRectF selectionBounds;
    if (m_selection_state.kind == SelectionKind::Primitive
        && m_selection_state.layer_index >= 0
        && m_selection_state.layer_index < m_document_data.layers.size())
    {
        if (!(suppressSelectedPrimitive
              && m_edit_preview_state.selection_state.layer_index == m_selection_state.layer_index
              && m_edit_preview_state.selection_state.primitive_index == m_selection_state.primitive_index))
        {
            selectionBounds = primitiveSelectionBounds(
                m_document_data.layers.at(m_selection_state.layer_index), m_selection_state.primitive_index);
        }
    }
    else if (
        m_selection_state.kind == SelectionKind::Layer
        && m_selection_state.layer_index >= 0
        && m_selection_state.layer_index < m_document_data.layers.size())
    {
        selectionBounds = layerSelectionBounds(m_document_data.layers.at(m_selection_state.layer_index));
    }

    if (!selectionBounds.isNull())
    {
        QPen selectionPen(renderColors.selection_stroke);
        selectionPen.setStyle(Qt::DashLine);
        selectionPen.setCosmetic(true);
        selectionPen.setWidthF(1.5);

        auto *overlay = addRect(selectionBounds, selectionPen, QBrush(Qt::NoBrush));
        overlay->setData(kSelectionOverlayRole, true);
        overlay->setAcceptedMouseButtons(Qt::NoButton);
        overlay->setZValue(10000.0);
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
    const RenderColors &renderColors = RenderTheme::colors();
    QPen gridPen(renderColors.grid_line);
    gridPen.setWidthF(0.0);

    // Use a slightly darker pen for the origin axes.
    QPen axisPen(renderColors.axis_line);
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
