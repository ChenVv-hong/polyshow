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
LayerData createEmptyLayer(
    const QString &displayName, LayerType layerType, const QString &filePath = QString());

/// Finds one layer by its document-unique display name.
[[nodiscard]]
LayerData *findLayerByName(DocumentData &documentData, const QString &layerName);

/// Finds one layer by its document-unique display name.
[[nodiscard]]
const LayerData *findLayerByName(const DocumentData &documentData, const QString &layerName);

/// Appends one primitive to the target layer and returns the runtime primitive index.
[[nodiscard]]
bool appendPrimitiveToLayer(
    LayerData &layer,
    const PrimitiveWriteRequest &request,
    int *primitiveIndex = nullptr,
    QString *errorMessage = nullptr);

/// Replaces one named primitive in-place or appends it when the name is not present.
[[nodiscard]]
bool replaceOrAppendNamedPrimitive(
    LayerData &layer,
    const PrimitiveWriteRequest &request,
    QString *resultMessage = nullptr,
    bool *replaced = nullptr);

/// Writes one primitive into a named IPC layer, enforcing the IPC-only target rule.
[[nodiscard]]
bool writePrimitiveToNamedIpcLayer(
    DocumentData &documentData,
    const QString &layerName,
    const PrimitiveWriteRequest &request,
    QString *resultMessage = nullptr,
    bool *replaced = nullptr);

} // namespace PolyShow
