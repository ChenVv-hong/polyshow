#pragma once

#include "core/GeometryTypes.h"

#include <QString>

namespace PolyShow
{

/// Serializes one runtime layer back into PolyShow's custom `.ply` format.
class PlySerializer final
{
public:
    /// Builds the full text payload for one layer.
    [[nodiscard]]
    static QString serializeLayer(const LayerData &layer);

    /// Writes one layer to disk as a `.ply` text file.
    [[nodiscard]]
    static bool writeLayerFile(const LayerData &layer, const QString &filePath, QString *errorMessage = nullptr);
};

} // namespace PolyShow
