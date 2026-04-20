#pragma once

#include "core/GeometryTypes.h"

#include <QMetaType>

namespace PolyShow
{

/// Identifies one style field editable from the inspector.
enum class PrimitiveStyleField
{
    StrokeColor,
    FillColor,
    Width,
    PointSize,
    FillEnabled
};

/// Carries one field-level style edit coming from the inspector.
struct PrimitiveStyleChangeRequest
{
    /// Selected primitive kind.
    PrimitiveKind primitive_kind {PrimitiveKind::Point};

    /// Style field being updated.
    PrimitiveStyleField field {PrimitiveStyleField::StrokeColor};

    /// Selected layer index.
    int layer_index {-1};

    /// Selected primitive index within the layer primitive list.
    int primitive_index {-1};

    /// Text value for string or numeric fields.
    QString text_value;

    /// Boolean value for toggle fields.
    bool bool_value {false};
};

/// Carries one real-time coordinate draft for a selected primitive.
struct PrimitiveCoordinateDraft
{
    /// Selected primitive kind.
    PrimitiveKind primitive_kind {PrimitiveKind::Point};

    /// Selected layer index.
    int layer_index {-1};

    /// Selected primitive index within the layer primitive list.
    int primitive_index {-1};

    /// Coordinates text using one `x y` pair per line.
    QString coordinates_text;
};

/// Stores the parsed primitive values produced by validation helpers.
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

/// Stores one scene-level preview suppression state for inspector drafts.
struct PrimitiveEditPreviewState
{
    /// Selection that owns the preview state.
    SelectionState selection_state;

    /// Whether the selected primitive should be temporarily hidden.
    bool hide_selected_primitive {false};

    [[nodiscard]]
    bool operator==(const PrimitiveEditPreviewState &other) const
    {
        return selection_state == other.selection_state && hide_selected_primitive == other.hide_selected_primitive;
    }

    [[nodiscard]]
    bool operator!=(const PrimitiveEditPreviewState &other) const
    {
        return !(*this == other);
    }
};

/// Returns the style stored by one layer primitive entry.
[[nodiscard]]
PrimitiveStyle primitiveStyle(const LayerData &layer, int primitiveIndex);

/// Returns the point list stored by one layer primitive entry.
[[nodiscard]]
QVector<Point2D> primitivePoints(const LayerData &layer, int primitiveIndex);

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

/// Validates one field-level style request and applies it to parsed values.
[[nodiscard]]
bool validateStyleChangeRequest(
    const LayerData &layer,
    int primitiveIndex,
    const PrimitiveStyleChangeRequest &request,
    PrimitiveEditValues *values,
    QString *errorMessage = nullptr);

/// Validates one real-time coordinate draft and parses its point list.
[[nodiscard]]
bool validateCoordinateDraft(
    const LayerData &layer,
    int primitiveIndex,
    const PrimitiveCoordinateDraft &draft,
    QVector<Point2D> *points,
    QString *errorMessage = nullptr);

/// Formats the editable coordinate list for one primitive.
[[nodiscard]]
QString formatCoordinateText(const QVector<Point2D> &points);

/// Formats one color using `#RRGGBB` or `#RRGGBBAA`.
[[nodiscard]]
QString formatColorText(const QColor &color);

/// Returns the user-facing label for one editable style field.
[[nodiscard]]
QString primitiveStyleFieldText(PrimitiveStyleField field);

} // namespace PolyShow

Q_DECLARE_METATYPE(PolyShow::PrimitiveStyleChangeRequest)
Q_DECLARE_METATYPE(PolyShow::PrimitiveCoordinateDraft)
