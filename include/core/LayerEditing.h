#pragma once

#include "core/GeometryTypes.h"

#include <QString>
#include <QVector>

namespace PolyShow
{

/// Describes one primitive append request targeting a specific layer.
struct PrimitiveWriteRequest
{
    /// Primitive shape category.
    PrimitiveKind kind {PrimitiveKind::Point};

    /// Primitive vertices or point coordinate.
    QVector<Point2D> points;

    /// Style payload written into the layer geometry.
    PrimitiveStyle style;

    /// Optional persisted primitive name.
    QString name;

    /// Initial primitive visibility.
    bool visible {true};
};

/// Creates one empty editable layer.
[[nodiscard]]
LayerData createEmptyLayer(const QString &displayName, const QString &filePath = QString());

/// Appends one primitive to the target layer and returns the runtime primitive index.
[[nodiscard]]
bool appendPrimitiveToLayer(
    LayerData &layer,
    const PrimitiveWriteRequest &request,
    int *primitiveIndex = nullptr,
    QString *errorMessage = nullptr);

} // namespace PolyShow
