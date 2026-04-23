#include "core/LayerEditing.h"

#include "core/PrimitiveEditing.h"

#include <utility>

namespace PolyShow
{

namespace
{

struct OrderedPrimitiveDescriptor
{
    PrimitiveWriteRequest request;
};

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

QString storedPrimitiveName(const LayerData &layer, int primitiveIndex)
{
    if (primitiveIndex < 0 || primitiveIndex >= layer.primitives.size())
    {
        return {};
    }

    const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
    switch (primitive.reference.kind)
    {
    case PrimitiveKind::Point:
        return layer.geometry.points.value(primitive.reference.index).name;
    case PrimitiveKind::Polyline:
        return layer.geometry.polylines.value(primitive.reference.index).name;
    case PrimitiveKind::Polygon:
        return layer.geometry.polygons.value(primitive.reference.index).name;
    default:
        return {};
    }
}

bool layerHasVisiblePrimitives(const LayerData &layer)
{
    if (layer.primitives.isEmpty())
    {
        return true;
    }

    for (const LayerPrimitiveData &primitive : layer.primitives)
    {
        if (primitive.visible)
        {
            return true;
        }
    }

    return false;
}

bool readPrimitiveDescriptor(const LayerData &layer, int primitiveIndex, OrderedPrimitiveDescriptor *descriptor)
{
    if (descriptor == nullptr || primitiveIndex < 0 || primitiveIndex >= layer.primitives.size())
    {
        return false;
    }

    PrimitiveEditValues editValues;
    if (!readPrimitiveEditValues(layer, primitiveIndex, &editValues))
    {
        return false;
    }

    const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
    descriptor->request.kind = primitive.reference.kind;
    descriptor->request.points = editValues.points;
    descriptor->request.style = editValues.style;
    descriptor->request.name = storedPrimitiveName(layer, primitiveIndex);
    descriptor->request.visible = primitive.visible;
    return true;
}

bool rebuildLayerFromOrderedDescriptors(
    LayerData &layer, const QVector<OrderedPrimitiveDescriptor> &descriptors, QString *errorMessage)
{
    LayerData rebuiltLayer = createEmptyLayer(layer.display_name, layer.layer_type, layer.file_path);

    for (const OrderedPrimitiveDescriptor &descriptor : descriptors)
    {
        QString appendError;
        if (!appendPrimitiveToLayer(rebuiltLayer, descriptor.request, nullptr, &appendError))
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = appendError;
            }
            return false;
        }
    }

    rebuiltLayer.visible = layerHasVisiblePrimitives(rebuiltLayer);
    layer = std::move(rebuiltLayer);
    return true;
}

} // namespace

LayerData createEmptyLayer(const QString &displayName, LayerType layerType, const QString &filePath)
{
    LayerData layer;
    layer.file_path = filePath;
    layer.display_name = displayName;
    layer.layer_type = layerType;
    layer.visible = true;
    return layer;
}

LayerData *findLayerByName(DocumentData &documentData, const QString &layerName)
{
    for (LayerData &layer : documentData.layers)
    {
        if (layer.display_name == layerName)
        {
            return &layer;
        }
    }

    return nullptr;
}

const LayerData *findLayerByName(const DocumentData &documentData, const QString &layerName)
{
    for (const LayerData &layer : documentData.layers)
    {
        if (layer.display_name == layerName)
        {
            return &layer;
        }
    }

    return nullptr;
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

bool replaceOrAppendNamedPrimitive(
    LayerData &layer, const PrimitiveWriteRequest &request, QString *resultMessage, bool *replaced)
{
    if (replaced != nullptr)
    {
        *replaced = false;
    }

    if (request.name.isEmpty())
    {
        QString errorMessage;
        if (!appendPrimitiveToLayer(layer, request, nullptr, &errorMessage))
        {
            if (resultMessage != nullptr)
            {
                *resultMessage = errorMessage;
            }
            return false;
        }

        if (resultMessage != nullptr)
        {
            *resultMessage = QStringLiteral("appended primitive");
        }
        return true;
    }

    int matchedPrimitiveIndex = -1;
    for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
    {
        if (storedPrimitiveName(layer, primitiveIndex) == request.name)
        {
            matchedPrimitiveIndex = primitiveIndex;
            break;
        }
    }

    if (matchedPrimitiveIndex < 0)
    {
        QString errorMessage;
        if (!appendPrimitiveToLayer(layer, request, nullptr, &errorMessage))
        {
            if (resultMessage != nullptr)
            {
                *resultMessage = errorMessage;
            }
            return false;
        }

        if (resultMessage != nullptr)
        {
            *resultMessage = QStringLiteral("appended primitive");
        }
        return true;
    }

    QVector<OrderedPrimitiveDescriptor> descriptors;
    descriptors.reserve(layer.primitives.size());
    for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
    {
        OrderedPrimitiveDescriptor descriptor;
        if (!readPrimitiveDescriptor(layer, primitiveIndex, &descriptor))
        {
            if (resultMessage != nullptr)
            {
                *resultMessage = QStringLiteral("Failed to rebuild ordered primitive descriptors.");
            }
            return false;
        }
        descriptors.append(descriptor);
    }

    PrimitiveWriteRequest replacementRequest = request;
    replacementRequest.visible = layer.primitives.at(matchedPrimitiveIndex).visible;
    descriptors[matchedPrimitiveIndex].request = replacementRequest;

    if (!rebuildLayerFromOrderedDescriptors(layer, descriptors, resultMessage))
    {
        return false;
    }

    if (replaced != nullptr)
    {
        *replaced = true;
    }

    if (resultMessage != nullptr)
    {
        *resultMessage = QStringLiteral("replaced primitive");
    }
    return true;
}

bool writePrimitiveToNamedIpcLayer(
    DocumentData &documentData,
    const QString &layerName,
    const PrimitiveWriteRequest &request,
    QString *resultMessage,
    bool *replaced)
{
    LayerData *layer = findLayerByName(documentData, layerName);
    if (layer == nullptr)
    {
        if (resultMessage != nullptr)
        {
            *resultMessage = QStringLiteral("Target layer \"%1\" does not exist.").arg(layerName);
        }
        return false;
    }

    if (layer->layer_type != LayerType::InternalIpc)
    {
        if (resultMessage != nullptr)
        {
            *resultMessage = QStringLiteral("Target layer \"%1\" is not an IPC layer.").arg(layerName);
        }
        return false;
    }

    return replaceOrAppendNamedPrimitive(*layer, request, resultMessage, replaced);
}

} // namespace PolyShow
