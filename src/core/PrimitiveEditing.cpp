#include "core/PrimitiveEditing.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>

namespace PolyShow
{

namespace
{

constexpr double kPointEqualityEpsilon = 1e-9;

/// Parses one hex color byte.
bool parseHexByte(const QString &text, int &value)
{
    bool ok = false;
    const int parsed = text.toInt(&ok, 16);
    if (!ok)
    {
        return false;
    }

    value = parsed;
    return true;
}

/// Returns whether one primitive list index points at a valid entry.
bool isValidPrimitiveIndex(const LayerData &layer, int primitiveIndex)
{
    return primitiveIndex >= 0 && primitiveIndex < layer.primitives.size();
}

/// Returns the geometry reference for one primitive list entry.
const LayerPrimitiveData *layerPrimitive(const LayerData &layer, int primitiveIndex)
{
    if (!isValidPrimitiveIndex(layer, primitiveIndex))
    {
        return nullptr;
    }

    return &layer.primitives.at(primitiveIndex);
}

/// Returns the formatted numeric text used by editable controls.
QString formatNumber(double value)
{
    return QString::number(value, 'f', 2);
}

/// Applies one validation error string if the destination exists.
void assignError(QString *target, const QString &message)
{
    if (target != nullptr)
    {
        *target = message;
    }
}

/// Builds a coordinate error with an optional source line number.
QString coordinateErrorText(const QString &message, int errorLine)
{
    return errorLine > 0 ? QStringLiteral("Line %1: %2").arg(errorLine).arg(message) : message;
}

} // namespace

PrimitiveStyle primitiveStyle(const LayerData &layer, int primitiveIndex)
{
    const LayerPrimitiveData *primitive = layerPrimitive(layer, primitiveIndex);
    if (primitive == nullptr)
    {
        return {};
    }

    switch (primitive->reference.kind)
    {
    case PrimitiveKind::Point:
        return layer.geometry.points.value(primitive->reference.index).style;
    case PrimitiveKind::Polyline:
        return layer.geometry.polylines.value(primitive->reference.index).style;
    case PrimitiveKind::Polygon:
        return layer.geometry.polygons.value(primitive->reference.index).style;
    default:
        return {};
    }
}

QVector<Point2D> primitivePoints(const LayerData &layer, int primitiveIndex)
{
    const LayerPrimitiveData *primitive = layerPrimitive(layer, primitiveIndex);
    if (primitive == nullptr)
    {
        return {};
    }

    switch (primitive->reference.kind)
    {
    case PrimitiveKind::Point:
        return QVector<Point2D> {layer.geometry.points.value(primitive->reference.index).point};
    case PrimitiveKind::Polyline:
        return layer.geometry.polylines.value(primitive->reference.index).vertices;
    case PrimitiveKind::Polygon:
        return layer.geometry.polygons.value(primitive->reference.index).vertices;
    default:
        return {};
    }
}

bool readPrimitiveEditValues(const LayerData &layer, int primitiveIndex, PrimitiveEditValues *values)
{
    if (values == nullptr)
    {
        return false;
    }

    const LayerPrimitiveData *primitive = layerPrimitive(layer, primitiveIndex);
    if (primitive == nullptr)
    {
        return false;
    }

    values->style = primitiveStyle(layer, primitiveIndex);
    values->points = primitivePoints(layer, primitiveIndex);
    return true;
}

bool applyPrimitiveEditValues(LayerData &layer, int primitiveIndex, const PrimitiveEditValues &values)
{
    if (!isValidPrimitiveIndex(layer, primitiveIndex))
    {
        return false;
    }

    const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
    switch (primitive.reference.kind)
    {
    case PrimitiveKind::Point:
        if (values.points.size() != 1 || primitive.reference.index < 0
            || primitive.reference.index >= layer.geometry.points.size())
        {
            return false;
        }
        layer.geometry.points[primitive.reference.index].point = values.points.first();
        layer.geometry.points[primitive.reference.index].style = values.style;
        return true;
    case PrimitiveKind::Polyline:
        if (primitive.reference.index < 0 || primitive.reference.index >= layer.geometry.polylines.size())
        {
            return false;
        }
        layer.geometry.polylines[primitive.reference.index].vertices = values.points;
        layer.geometry.polylines[primitive.reference.index].style = values.style;
        return true;
    case PrimitiveKind::Polygon:
        if (primitive.reference.index < 0 || primitive.reference.index >= layer.geometry.polygons.size())
        {
            return false;
        }
        layer.geometry.polygons[primitive.reference.index].vertices = values.points;
        layer.geometry.polygons[primitive.reference.index].style = values.style;
        return true;
    default:
        return false;
    }
}

bool parseDoubleText(const QString &text, double &value)
{
    bool ok = false;
    const double parsed = text.toDouble(&ok);
    if (!ok)
    {
        return false;
    }

    value = parsed;
    return true;
}

bool parsePositiveDoubleText(const QString &text, double &value)
{
    if (!parseDoubleText(text, value) || value <= 0.0)
    {
        return false;
    }

    return true;
}

bool parseColorText(const QString &text, QColor &color)
{
    const QString trimmed = text.trimmed();
    if (!trimmed.startsWith('#'))
    {
        return false;
    }

    const QString hex = trimmed.mid(1);
    if (hex.size() != 6 && hex.size() != 8)
    {
        return false;
    }

    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 255;
    if (!parseHexByte(hex.mid(0, 2), red) || !parseHexByte(hex.mid(2, 2), green) || !parseHexByte(hex.mid(4, 2), blue))
    {
        return false;
    }

    if (hex.size() == 8 && !parseHexByte(hex.mid(6, 2), alpha))
    {
        return false;
    }

    color = QColor(red, green, blue, alpha);
    return true;
}

bool pointsEqual(const Point2D &lhs, const Point2D &rhs)
{
    return std::abs(lhs.x - rhs.x) <= kPointEqualityEpsilon && std::abs(lhs.y - rhs.y) <= kPointEqualityEpsilon;
}

int uniquePointCount(const QVector<Point2D> &points, bool ignoreClosingPoint)
{
    const int pointCount = static_cast<int>(points.size());
    const int lastIndex = ignoreClosingPoint ? std::max(pointCount - 1, 0) : pointCount;
    QVector<Point2D> uniquePoints;
    uniquePoints.reserve(lastIndex);

    for (int index = 0; index < lastIndex; ++index)
    {
        bool found = false;
        for (const Point2D &existing : std::as_const(uniquePoints))
        {
            if (pointsEqual(existing, points.at(index)))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            uniquePoints.append(points.at(index));
        }
    }

    return uniquePoints.size();
}

bool parseCoordinateText(const QString &text, QVector<Point2D> &points, QString *errorMessage, int *errorLine)
{
    points.clear();

    const QStringList lines = text.split('\n');
    for (int lineNumber = 0; lineNumber < lines.size(); ++lineNumber)
    {
        const QString trimmedLine = lines.at(lineNumber).trimmed();
        if (trimmedLine.isEmpty())
        {
            continue;
        }

        const QStringList parts = trimmedLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() != 2)
        {
            assignError(errorMessage, QStringLiteral("Each coordinate line must contain exactly two numbers."));
            if (errorLine != nullptr)
            {
                *errorLine = lineNumber + 1;
            }
            return false;
        }

        double x = 0.0;
        double y = 0.0;
        if (!parseDoubleText(parts.at(0), x) || !parseDoubleText(parts.at(1), y))
        {
            assignError(errorMessage, QStringLiteral("Failed to parse coordinate values."));
            if (errorLine != nullptr)
            {
                *errorLine = lineNumber + 1;
            }
            return false;
        }

        points.append(Point2D {x, y});
    }

    return true;
}

bool validatePrimitivePoints(PrimitiveKind kind, const QVector<Point2D> &points, QString *errorMessage)
{
    if (kind == PrimitiveKind::Point)
    {
        if (points.size() != 1)
        {
            assignError(errorMessage, QStringLiteral("Point primitives require exactly one coordinate."));
            return false;
        }
        return true;
    }

    if (kind == PrimitiveKind::Polyline)
    {
        if (points.size() < 2)
        {
            assignError(errorMessage, QStringLiteral("Polyline primitives require at least two coordinates."));
            return false;
        }

        return true;
    }

    if (points.size() < 3)
    {
        assignError(errorMessage, QStringLiteral("Polygon primitives require at least three coordinates."));
        return false;
    }

    return true;
}

bool validateStyleChangeRequest(
    const LayerData &layer,
    int primitiveIndex,
    const PrimitiveStyleChangeRequest &request,
    PrimitiveEditValues *values,
    QString *errorMessage)
{
    PrimitiveEditValues parsedValues;
    if (!readPrimitiveEditValues(layer, primitiveIndex, &parsedValues))
    {
        return false;
    }

    if (request.primitive_kind != layer.primitives.at(primitiveIndex).reference.kind)
    {
        assignError(errorMessage, QStringLiteral("Selection kind changed."));
        return false;
    }

    switch (request.field)
    {
    case PrimitiveStyleField::StrokeColor:
        if (!parseColorText(request.text_value, parsedValues.style.color))
        {
            assignError(errorMessage, QStringLiteral("Use #RRGGBB or #RRGGBBAA."));
            return false;
        }
        break;
    case PrimitiveStyleField::FillColor:
        if (request.primitive_kind != PrimitiveKind::Polygon)
        {
            assignError(errorMessage, QStringLiteral("Fill color is only available for polygons."));
            return false;
        }
        if (!parseColorText(request.text_value, parsedValues.style.fill_color))
        {
            assignError(errorMessage, QStringLiteral("Use #RRGGBB or #RRGGBBAA."));
            return false;
        }
        break;
    case PrimitiveStyleField::Width:
        if (request.primitive_kind == PrimitiveKind::Point)
        {
            assignError(errorMessage, QStringLiteral("Line width is not available for points."));
            return false;
        }
        if (!parsePositiveDoubleText(request.text_value, parsedValues.style.width))
        {
            assignError(errorMessage, QStringLiteral("Width must be greater than 0."));
            return false;
        }
        break;
    case PrimitiveStyleField::PointSize:
        if (!parsePositiveDoubleText(request.text_value, parsedValues.style.point_size))
        {
            assignError(errorMessage, QStringLiteral("Point size must be greater than 0."));
            return false;
        }
        break;
    case PrimitiveStyleField::FillEnabled:
        if (request.primitive_kind != PrimitiveKind::Polygon)
        {
            assignError(errorMessage, QStringLiteral("Fill is only available for polygons."));
            return false;
        }
        parsedValues.style.fill_enabled = request.bool_value;
        break;
    default:
        assignError(errorMessage, QStringLiteral("Unsupported style field."));
        return false;
    }

    if (values != nullptr)
    {
        *values = parsedValues;
    }
    return true;
}

bool validateCoordinateDraft(
    const LayerData &layer,
    int primitiveIndex,
    const PrimitiveCoordinateDraft &draft,
    QVector<Point2D> *points,
    QString *errorMessage)
{
    if (!isValidPrimitiveIndex(layer, primitiveIndex))
    {
        return false;
    }

    if (draft.primitive_kind != layer.primitives.at(primitiveIndex).reference.kind)
    {
        assignError(errorMessage, QStringLiteral("Selection kind changed."));
        return false;
    }

    QVector<Point2D> parsedPoints;
    QString parseError;
    int errorLine = -1;
    if (!parseCoordinateText(draft.coordinates_text, parsedPoints, &parseError, &errorLine))
    {
        assignError(errorMessage, coordinateErrorText(parseError, errorLine));
        return false;
    }

    QString geometryError;
    if (!validatePrimitivePoints(draft.primitive_kind, parsedPoints, &geometryError))
    {
        assignError(errorMessage, geometryError);
        return false;
    }

    if (points != nullptr)
    {
        *points = parsedPoints;
    }
    return true;
}

QString formatCoordinateText(const QVector<Point2D> &points)
{
    QStringList lines;
    lines.reserve(points.size());

    for (const Point2D &point : points)
    {
        lines.append(QStringLiteral("%1 %2").arg(formatNumber(point.x)).arg(formatNumber(point.y)));
    }

    return lines.join(QStringLiteral("\n"));
}

QString formatColorText(const QColor &color)
{
    const auto toHex = [](int value) {
        return QStringLiteral("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper();
    };

    const QString base = QStringLiteral("#%1%2%3").arg(toHex(color.red()), toHex(color.green()), toHex(color.blue()));
    if (color.alpha() == 255)
    {
        return base;
    }

    return QStringLiteral("%1%2").arg(base, toHex(color.alpha()));
}

QString primitiveStyleFieldText(PrimitiveStyleField field)
{
    switch (field)
    {
    case PrimitiveStyleField::StrokeColor:
        return QStringLiteral("stroke color");
    case PrimitiveStyleField::FillColor:
        return QStringLiteral("fill color");
    case PrimitiveStyleField::Width:
        return QStringLiteral("line width");
    case PrimitiveStyleField::PointSize:
        return QStringLiteral("point size");
    case PrimitiveStyleField::FillEnabled:
        return QStringLiteral("fill");
    default:
        return QStringLiteral("style field");
    }
}

} // namespace PolyShow
