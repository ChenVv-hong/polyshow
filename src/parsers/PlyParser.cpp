#include "parsers/PlyParser.h"

#include "core/PrimitiveEditing.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

#include <utility>

namespace PolyShow
{

namespace
{

/// Tracks the inheritable style state while parsing one shape block.
struct StyleConfig
{
    QColor color {34, 89, 180};
    QColor fill_color {34, 89, 180, 80};
    bool fill_enabled {true};
    bool fill_follows_color {true};
    double width {1.5};
    double point_size {2.5};

    /// Converts the parser state to the runtime style type.
    [[nodiscard]]
    PrimitiveStyle toPrimitiveStyle() const
    {
        return PrimitiveStyle {color, fill_color, fill_enabled, width, point_size};
    }
};

/// Formats an error message with the source line number.
QString lineError(int lineNumber, const QString &message)
{
    return QStringLiteral("Line %1: %2").arg(lineNumber).arg(message);
}

/// Builds the default translucent fill color from the stroke color.
QColor translucentFillColor(const QColor &color)
{
    QColor fillColor = color;
    fillColor.setAlpha(80);
    return fillColor;
}

/// Returns whether the token is a supported style directive.
bool isDirectiveToken(const QString &token)
{
    return token == QStringLiteral("COLOR") || token == QStringLiteral("FILL") || token == QStringLiteral("WIDTH")
        || token == QStringLiteral("POINT_SIZE");
}

/// Parses and applies one style directive.
bool applyDirective(
    const QStringList &parts, StyleConfig &style, int lineNumber, bool shapeStarted, QString &errorMessage)
{
    // Style directives must appear before any vertex in the current shape block.
    if (shapeStarted)
    {
        errorMessage = lineError(lineNumber, QStringLiteral("Style directives must appear before shape coordinates"));
        return false;
    }

    if (parts.size() != 2)
    {
        errorMessage = lineError(lineNumber, QStringLiteral("Directive requires exactly one value"));
        return false;
    }

    const QString &directive = parts[0];
    const QString &value = parts[1];
    if (directive == QStringLiteral("COLOR"))
    {
        QColor color;
        if (!parseColorText(value, color))
        {
            errorMessage = lineError(lineNumber, QStringLiteral("Invalid COLOR value: %1").arg(value));
            return false;
        }

        style.color = color;
        if (style.fill_follows_color)
        {
            // Keep fill synchronized with COLOR until FILL is explicitly overridden.
            style.fill_enabled = true;
            style.fill_color = translucentFillColor(color);
        }
        return true;
    }

    if (directive == QStringLiteral("FILL"))
    {
        if (value == QStringLiteral("none"))
        {
            style.fill_enabled = false;
            style.fill_follows_color = false;
            return true;
        }

        QColor fillColor;
        if (!parseColorText(value, fillColor))
        {
            errorMessage = lineError(lineNumber, QStringLiteral("Invalid FILL value: %1").arg(value));
            return false;
        }

        // Once fill is explicitly set, future COLOR directives must not override it.
        style.fill_enabled = true;
        style.fill_follows_color = false;
        style.fill_color = fillColor;
        return true;
    }

    double number = 0.0;
    if (!parseDoubleText(value, number))
    {
        errorMessage = lineError(lineNumber, QStringLiteral("Invalid numeric value: %1").arg(value));
        return false;
    }

    if (number <= 0.0)
    {
        errorMessage = lineError(lineNumber, QStringLiteral("Numeric value must be greater than 0"));
        return false;
    }

    if (directive == QStringLiteral("WIDTH"))
    {
        style.width = number;
        return true;
    }

    if (directive == QStringLiteral("POINT_SIZE"))
    {
        style.point_size = number;
        return true;
    }

    errorMessage = lineError(lineNumber, QStringLiteral("Unknown directive: %1").arg(directive));
    return false;
}

/// Finalizes the current vertex block into a point, polyline, or polygon.
bool finalizeShape(
    const QVector<Point2D> &vertices,
    const StyleConfig &style,
    int lineNumber,
    GeometryData &data,
    QString &errorMessage)
{
    if (vertices.isEmpty())
    {
        return true;
    }

    const PrimitiveStyle primitiveStyle = style.toPrimitiveStyle();
    if (vertices.size() == 1)
    {
        data.points.append(PointShape2D {vertices.first(), primitiveStyle});
        data.primitive_order.append(
            PrimitiveReference {PrimitiveKind::Point, static_cast<int>(data.points.size() - 1)});
        return true;
    }

    // Distinguish open and closed shapes by comparing the first and last point.
    const bool isClosed = pointsEqual(vertices.first(), vertices.last());
    const int uniqueCount = uniquePointCount(vertices, isClosed);
    if (uniqueCount < 2)
    {
        errorMessage = lineError(lineNumber, QStringLiteral("Degenerate shape contains only repeated points"));
        return false;
    }

    if (isClosed)
    {
        if (vertices.size() < 4)
        {
            errorMessage = lineError(
                lineNumber, QStringLiteral("Closed shape needs at least 4 points including the repeated start point"));
            return false;
        }

        if (uniqueCount < 3)
        {
            errorMessage = lineError(lineNumber, QStringLiteral("Polygon needs at least 3 unique vertices"));
            return false;
        }

        // Remove the duplicated closing point before storing the polygon vertices.
        Polygon2D polygon;
        polygon.style = primitiveStyle;
        polygon.vertices = vertices.mid(0, vertices.size() - 1);
        data.polygons.append(polygon);
        data.primitive_order.append(
            PrimitiveReference {PrimitiveKind::Polygon, static_cast<int>(data.polygons.size() - 1)});
        return true;
    }

    Polyline2D polyline;
    polyline.style = primitiveStyle;
    polyline.vertices = vertices;
    data.polylines.append(polyline);
    data.primitive_order.append(
        PrimitiveReference {PrimitiveKind::Polyline, static_cast<int>(data.polylines.size() - 1)});
    return true;
}

} // namespace

/// Parses the custom PolyShow `.ply` text file format.
bool PlyParser::parseFile(const QString &filePath, GeometryData &geometryData, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Failed to open file: %1").arg(filePath);
        }
        return false;
    }

    QTextStream input(&file);
    GeometryData parsed;
    QVector<Point2D> currentVertices;
    StyleConfig currentStyle;
    int lineNumber = 0;
    int currentShapeLine = 0;

    // Scan the file line by line while preserving the active shape and style state.
    while (!input.atEnd())
    {
        const QString rawLine = input.readLine();
        ++lineNumber;

        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#'))
        {
            continue;
        }

        if (line == QStringLiteral("NEXT"))
        {
            QString finalizeError;
            const int shapeLine = currentShapeLine == 0 ? lineNumber : currentShapeLine;

            // Commit the current shape block when a block separator is reached.
            if (!finalizeShape(currentVertices, currentStyle, shapeLine, parsed, finalizeError))
            {
                if (errorMessage != nullptr)
                {
                    *errorMessage = finalizeError;
                }
                return false;
            }

            currentVertices.clear();
            currentShapeLine = 0;
            continue;
        }

        const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.isEmpty())
        {
            continue;
        }

        // Reject unknown word-like directives as early as possible.
        if (!parts[0].isEmpty() && parts[0].at(0).isLetter() && !isDirectiveToken(parts[0]))
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = lineError(lineNumber, QStringLiteral("Unknown directive: %1").arg(parts[0]));
            }
            return false;
        }

        if (isDirectiveToken(parts[0]))
        {
            QString directiveError;

            // Style directives affect the current parser state but do not emit geometry directly.
            if (!applyDirective(parts, currentStyle, lineNumber, !currentVertices.isEmpty(), directiveError))
            {
                if (errorMessage != nullptr)
                {
                    *errorMessage = directiveError;
                }
                return false;
            }
            continue;
        }

        if (parts.size() != 2)
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = lineError(lineNumber, QStringLiteral("Coordinate line must contain exactly two numbers"));
            }
            return false;
        }

        double x = 0.0;
        double y = 0.0;
        if (!parseDoubleText(parts[0], x) || !parseDoubleText(parts[1], y))
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = lineError(lineNumber, QStringLiteral("Failed to parse coordinate values"));
            }
            return false;
        }

        if (currentVertices.isEmpty())
        {
            currentShapeLine = lineNumber;
        }
        currentVertices.append(Point2D {x, y});
    }

    // Flush the final shape block even when the file does not end with `NEXT`.
    QString finalizeError;
    const int shapeLine = currentShapeLine == 0 ? lineNumber : currentShapeLine;
    if (!finalizeShape(currentVertices, currentStyle, shapeLine, parsed, finalizeError))
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = finalizeError;
        }
        return false;
    }

    geometryData = parsed;
    return true;
}

} // namespace PolyShow
