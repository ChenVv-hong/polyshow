#pragma once

#include <QColor>
#include <QPointF>
#include <QVector>

namespace PolyShow
{

/// Represents a point in 2D world space.
struct Point2D
{
    /// X coordinate.
    double x {0.0};

    /// Y coordinate.
    double y {0.0};

    /// Converts the project point type to Qt's point type.
    [[nodiscard]]
    QPointF toPointF() const
    {
        // Keep Qt conversion logic in one place to avoid repeated call-site code.
        return QPointF(x, y);
    }
};

/// Shared render style used by points, polylines, and polygons.
struct PrimitiveStyle
{
    /// Main stroke or point color.
    QColor color {34, 89, 180};

    /// Polygon fill color.
    QColor fill_color {34, 89, 180, 80};

    /// Whether polygon fill is enabled.
    bool fill_enabled {true};

    /// Stroke width.
    double width {1.5};

    /// Point marker radius.
    double point_size {2.5};
};

/// Standalone point primitive.
struct PointShape2D
{
    /// Point position.
    Point2D point;

    /// Point render style.
    PrimitiveStyle style;
};

/// Open polyline primitive.
struct Polyline2D
{
    /// Ordered polyline vertices.
    QVector<Point2D> vertices;

    /// Polyline render style.
    PrimitiveStyle style;
};

/// Closed polygon primitive.
struct Polygon2D
{
    /// Ordered polygon vertices.
    QVector<Point2D> vertices;

    /// Polygon render style.
    PrimitiveStyle style;
};

/// Complete geometry payload produced by the parser.
struct GeometryData
{
    /// Standalone points.
    QVector<PointShape2D> points;

    /// Polylines.
    QVector<Polyline2D> polylines;

    /// Polygons.
    QVector<Polygon2D> polygons;

    /// Returns whether all primitive collections are empty.
    [[nodiscard]]
    bool isEmpty() const
    {
        // The geometry is empty only when every primitive bucket is empty.
        return points.isEmpty() && polylines.isEmpty() && polygons.isEmpty();
    }
};

} // namespace PolyShow
