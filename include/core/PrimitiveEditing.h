#pragma once

#include "core/GeometryTypes.h"

#include <QMetaType>

namespace PolyShow
{

/// Carries one inspector edit submission for a selected primitive.
struct PrimitiveEditRequest
{
    /// Selected primitive kind.
    PrimitiveKind primitive_kind {PrimitiveKind::Point};

    /// Selected layer index.
    int layer_index {-1};

    /// Selected primitive index within the layer primitive list.
    int primitive_index {-1};

    /// Stroke color text in `#RRGGBB` or `#RRGGBBAA` format.
    QString stroke_color_text;

    /// Fill color text in `#RRGGBB` or `#RRGGBBAA` format.
    QString fill_color_text;

    /// Stroke width text.
    QString width_text;

    /// Point size text.
    QString point_size_text;

    /// Coordinates text using one `x y` pair per line.
    QString coordinates_text;

    /// Whether polygon fill should be enabled.
    bool fill_enabled {true};

    [[nodiscard]]
    bool operator==(const PrimitiveEditRequest &other) const
    {
        return primitive_kind == other.primitive_kind && layer_index == other.layer_index
            && primitive_index == other.primitive_index && stroke_color_text == other.stroke_color_text
            && fill_color_text == other.fill_color_text && width_text == other.width_text
            && point_size_text == other.point_size_text && coordinates_text == other.coordinates_text
            && fill_enabled == other.fill_enabled;
    }

    [[nodiscard]]
    bool operator!=(const PrimitiveEditRequest &other) const
    {
        return !(*this == other);
    }
};

/// Stores the parsed primitive values produced by inspector validation.
struct PrimitiveEditValues
{
    /// Parsed style values.
    PrimitiveStyle style;

    /// Parsed primitive vertices.
    QVector<Point2D> points;
};

/// Holds field-level validation errors for the inspector form.
struct PrimitiveEditValidationErrors
{
    /// Error for the stroke color field.
    QString stroke_color;

    /// Error for the fill color field.
    QString fill_color;

    /// Error for the width field.
    QString width;

    /// Error for the point size field.
    QString point_size;

    /// Error for the coordinates editor.
    QString coordinates;

    /// Returns whether any field currently has an error.
    [[nodiscard]]
    bool hasErrors() const
    {
        return !stroke_color.isEmpty() || !fill_color.isEmpty() || !width.isEmpty() || !point_size.isEmpty()
            || !coordinates.isEmpty();
    }

    /// Clears every stored error string.
    void clear()
    {
        stroke_color.clear();
        fill_color.clear();
        width.clear();
        point_size.clear();
        coordinates.clear();
    }
};

/// Returns the style stored by one layer primitive entry.
[[nodiscard]]
PrimitiveStyle primitiveStyle(const LayerData &layer, int primitiveIndex);

/// Returns the point list stored by one layer primitive entry.
[[nodiscard]]
QVector<Point2D> primitivePoints(const LayerData &layer, int primitiveIndex);

/// Builds one inspector request from the current primitive state.
[[nodiscard]]
bool buildPrimitiveEditRequest(const LayerData &layer, int primitiveIndex, PrimitiveEditRequest *request);

/// Reads one primitive into parsed edit values.
[[nodiscard]]
bool readPrimitiveEditValues(const LayerData &layer, int primitiveIndex, PrimitiveEditValues *values);

/// Applies validated primitive values back into the layer geometry.
[[nodiscard]]
bool applyPrimitiveEditValues(LayerData &layer, int primitiveIndex, const PrimitiveEditValues &values);

/// Parses one floating-point value.
[[nodiscard]]
bool parseDoubleText(const QString &text, double &value);

/// Parses one positive floating-point value.
[[nodiscard]]
bool parsePositiveDoubleText(const QString &text, double &value);

/// Parses one color in `#RRGGBB` or `#RRGGBBAA` format.
[[nodiscard]]
bool parseColorText(const QString &text, QColor &color);

/// Returns whether the two points should be treated as equal.
[[nodiscard]]
bool pointsEqual(const Point2D &lhs, const Point2D &rhs);

/// Counts unique points, optionally ignoring a duplicated closing point.
[[nodiscard]]
int uniquePointCount(const QVector<Point2D> &points, bool ignoreClosingPoint = false);

/// Parses multi-line coordinate editor text into point values.
[[nodiscard]]
bool parseCoordinateText(
    const QString &text, QVector<Point2D> &points, QString *errorMessage = nullptr, int *errorLine = nullptr);

/// Validates the point list for one primitive kind.
[[nodiscard]]
bool validatePrimitivePoints(PrimitiveKind kind, const QVector<Point2D> &points, QString *errorMessage = nullptr);

/// Validates one inspector request and converts it into parsed values.
[[nodiscard]]
bool validatePrimitiveEditRequest(
    const LayerData &layer,
    int primitiveIndex,
    const PrimitiveEditRequest &request,
    PrimitiveEditValues *values,
    PrimitiveEditValidationErrors *errors);

/// Formats the editable coordinate list for one primitive.
[[nodiscard]]
QString formatCoordinateText(const QVector<Point2D> &points);

/// Formats one color using uppercase ARGB hex text.
[[nodiscard]]
QString formatColorText(const QColor &color);

} // namespace PolyShow

Q_DECLARE_METATYPE(PolyShow::PrimitiveEditRequest)
Q_DECLARE_METATYPE(PolyShow::PrimitiveEditValidationErrors)
