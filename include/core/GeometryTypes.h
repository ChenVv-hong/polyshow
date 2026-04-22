#pragma once

#include <QColor>
#include <QPointF>
#include <QString>
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

/// Identifies one primitive bucket in a parsed file.
enum class PrimitiveKind
{
    Point,
    Polyline,
    Polygon
};

/// Identifies what kind of UI selection is active.
enum class SelectionKind
{
    None,
    Layer,
    Primitive
};

/// References one primitive inside a `GeometryData` payload.
struct PrimitiveReference
{
    PrimitiveKind kind {PrimitiveKind::Point};
    int index {0};
};

/// Describes the currently selected layer or primitive.
struct SelectionState
{
    SelectionKind kind {SelectionKind::None};
    int layer_index {-1};
    int primitive_index {-1};

    [[nodiscard]]
    bool operator==(const SelectionState &other) const
    {
        return kind == other.kind && layer_index == other.layer_index && primitive_index == other.primitive_index;
    }

    [[nodiscard]]
    bool operator!=(const SelectionState &other) const
    {
        return !(*this == other);
    }
};

/// Standalone point primitive.
struct PointShape2D
{
    /// Point position.
    Point2D point;

    /// Point render style.
    PrimitiveStyle style;

    /// Optional user-defined display name.
    QString name;
};

/// Open polyline primitive.
struct Polyline2D
{
    /// Ordered polyline vertices.
    QVector<Point2D> vertices;

    /// Polyline render style.
    PrimitiveStyle style;

    /// Optional user-defined display name.
    QString name;
};

/// Closed polygon primitive.
struct Polygon2D
{
    /// Ordered polygon vertices.
    QVector<Point2D> vertices;

    /// Polygon render style.
    PrimitiveStyle style;

    /// Optional user-defined display name.
    QString name;
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

    /// Primitive order as encountered while parsing the file.
    QVector<PrimitiveReference> primitive_order;

    /// Returns whether all primitive collections are empty.
    [[nodiscard]]
    bool isEmpty() const
    {
        // The geometry is empty only when every primitive bucket is empty.
        return points.isEmpty() && polylines.isEmpty() && polygons.isEmpty();
    }
};

/// One layer-level primitive entry with independent visibility.
struct LayerPrimitiveData
{
    /// Reference back into the layer geometry payload.
    PrimitiveReference reference;

    /// User-facing primitive label.
    QString display_name;

    /// Whether the primitive is currently visible.
    bool visible {true};
};

/// One imported file plus an aggregate visibility summary.
struct LayerData
{
    /// Source file path used to build the layer.
    QString file_path;

    /// User-facing layer label.
    QString display_name;

    /// Parsed file geometry.
    GeometryData geometry;

    /// Flat primitive list in file order.
    QVector<LayerPrimitiveData> primitives;

    /// Aggregate visibility summary. True when at least one primitive is visible.
    bool visible {true};
};

/// Complete imported document containing multiple layers.
struct DocumentData
{
    /// Imported layers in load order.
    QVector<LayerData> layers;

    /// Returns whether the document has no layers.
    [[nodiscard]]
    bool isEmpty() const
    {
        return layers.isEmpty();
    }
};

} // namespace PolyShow
