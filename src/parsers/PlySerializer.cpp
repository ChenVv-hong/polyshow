#include "parsers/PlySerializer.h"

#include "core/PrimitiveEditing.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>

namespace PolyShow
{

namespace
{

QString formatCoordinate(double value)
{
    return QString::number(value, 'g', 15);
}

QString primitiveName(const LayerData &layer, int primitiveIndex)
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

QVector<Point2D> exportPoints(const LayerData &layer, int primitiveIndex)
{
    QVector<Point2D> points = primitivePoints(layer, primitiveIndex);
    if (primitiveIndex < 0 || primitiveIndex >= layer.primitives.size())
    {
        return points;
    }

    const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
    if (primitive.reference.kind == PrimitiveKind::Polygon && !points.isEmpty() && !pointsEqual(points.first(), points.last()))
    {
        points.append(points.first());
    }

    return points;
}

} // namespace

QString PlySerializer::serializeLayer(const LayerData &layer)
{
    QStringList blocks;
    blocks.reserve(layer.primitives.size());

    for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
    {
        const PrimitiveStyle style = primitiveStyle(layer, primitiveIndex);
        const QVector<Point2D> points = exportPoints(layer, primitiveIndex);
        if (points.isEmpty())
        {
            continue;
        }

        QStringList blockLines;
        const QString name = primitiveName(layer, primitiveIndex);
        if (!name.isEmpty())
        {
            blockLines.append(QStringLiteral("NAME %1").arg(name));
        }

        blockLines.append(QStringLiteral("COLOR %1").arg(formatColorText(style.color)));
        blockLines.append(
            style.fill_enabled
                ? QStringLiteral("FILL %1").arg(formatColorText(style.fill_color))
                : QStringLiteral("FILL none"));
        blockLines.append(QStringLiteral("WIDTH %1").arg(formatCoordinate(style.width)));
        blockLines.append(QStringLiteral("POINT_SIZE %1").arg(formatCoordinate(style.point_size)));

        for (const Point2D &point : points)
        {
            blockLines.append(QStringLiteral("%1 %2").arg(formatCoordinate(point.x), formatCoordinate(point.y)));
        }

        blocks.append(blockLines.join(QStringLiteral("\n")));
    }

    if (blocks.isEmpty())
    {
        return {};
    }

    return blocks.join(QStringLiteral("\n\nNEXT\n\n")) + QStringLiteral("\n");
}

bool PlySerializer::writeLayerFile(const LayerData &layer, const QString &filePath, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Failed to open file for writing: %1").arg(filePath);
        }
        return false;
    }

    QTextStream output(&file);
    output << serializeLayer(layer);
    if (output.status() != QTextStream::Ok)
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Failed to write file: %1").arg(filePath);
        }
        return false;
    }

    return true;
}

} // namespace PolyShow
