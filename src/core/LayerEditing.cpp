#include "core/LayerEditing.h"

#include "core/PrimitiveEditing.h"

#include <utility>

namespace PolyShow
{

namespace
{

QString primitiveCategoryLabel(PrimitiveKind kind, int vertexCount)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        return QStringLiteral("Point");
    case PrimitiveKind::Polyline:
        return vertexCount == 2 ? QStringLiteral("Line") : QStringLiteral("Polyline");
    case PrimitiveKind::Polygon:
        return QStringLiteral("Polygon");
    default:
        return QStringLiteral("Primitive");
    }
}

QVector<Point2D> normalizedRequestPoints(const PrimitiveWriteRequest &request)
{
    QVector<Point2D> points = request.points;
    if (request.kind == PrimitiveKind::Polygon && !points.isEmpty() && !pointsEqual(points.first(), points.last()))
    {
        points.append(points.first());
    }

    return points;
}

int nextPointOrdinal(const LayerData &layer)
{
    return layer.geometry.points.size();
}

int nextLineOrdinal(const LayerData &layer)
{
    int lineCount = 0;
    for (const Polyline2D &polyline : layer.geometry.polylines)
    {
        if (polyline.vertices.size() == 2)
        {
            ++lineCount;
        }
    }

    return lineCount;
}

int nextPolylineOrdinal(const LayerData &layer)
{
    int polylineCount = 0;
    for (const Polyline2D &polyline : layer.geometry.polylines)
    {
        if (polyline.vertices.size() != 2)
        {
            ++polylineCount;
        }
    }

    return polylineCount;
}

int nextPolygonOrdinal(const LayerData &layer)
{
    return layer.geometry.polygons.size();
}

QString generatedDisplayName(const LayerData &layer, PrimitiveKind kind, int vertexCount)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        return QStringLiteral("%1 %2").arg(primitiveCategoryLabel(kind, vertexCount)).arg(nextPointOrdinal(layer));
    case PrimitiveKind::Polyline:
        if (vertexCount == 2)
        {
            return QStringLiteral("%1 %2").arg(primitiveCategoryLabel(kind, vertexCount)).arg(nextLineOrdinal(layer));
        }
        return QStringLiteral("%1 %2").arg(primitiveCategoryLabel(kind, vertexCount)).arg(nextPolylineOrdinal(layer));
    case PrimitiveKind::Polygon:
        return QStringLiteral("%1 %2").arg(primitiveCategoryLabel(kind, vertexCount)).arg(nextPolygonOrdinal(layer));
    default:
        return QStringLiteral("Primitive");
    }
}

} // namespace

LayerData createEmptyLayer(const QString &displayName, const QString &filePath)
{
    LayerData layer;
    layer.file_path = filePath;
    layer.display_name = displayName;
    layer.visible = true;
    return layer;
}

bool appendPrimitiveToLayer(LayerData &layer, const PrimitiveWriteRequest &request, int *primitiveIndex, QString *errorMessage)
{
    const QVector<Point2D> points = normalizedRequestPoints(request);

    QString validationMessage;
    if (!validatePrimitivePoints(request.kind, points, &validationMessage))
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = validationMessage;
        }
        return false;
    }

    PrimitiveReference reference;
    switch (request.kind)
    {
    case PrimitiveKind::Point:
        layer.geometry.points.append(PointShape2D {points.first(), request.style, request.name});
        reference = PrimitiveReference {PrimitiveKind::Point, static_cast<int>(layer.geometry.points.size() - 1)};
        break;
    case PrimitiveKind::Polyline:
        layer.geometry.polylines.append(Polyline2D {points, request.style, request.name});
        reference = PrimitiveReference {PrimitiveKind::Polyline, static_cast<int>(layer.geometry.polylines.size() - 1)};
        break;
    case PrimitiveKind::Polygon:
        layer.geometry.polygons.append(Polygon2D {points, request.style, request.name});
        reference = PrimitiveReference {PrimitiveKind::Polygon, static_cast<int>(layer.geometry.polygons.size() - 1)};
        break;
    default:
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Unsupported primitive kind.");
        }
        return false;
    }

    layer.geometry.primitive_order.append(reference);
    const QString displayName = request.name.isEmpty()
        ? generatedDisplayName(layer, request.kind, points.size())
        : request.name;
    layer.primitives.append(LayerPrimitiveData {reference, displayName, request.visible});
    layer.visible = true;

    if (primitiveIndex != nullptr)
    {
        *primitiveIndex = layer.primitives.size() - 1;
    }

    return true;
}

} // namespace PolyShow
