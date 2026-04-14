#pragma once

#include "core/GeometryTypes.h"

#include <QString>

namespace PolyShow
{

/// Parses PolyShow's custom 2D `.ply` text format.
class PlyParser final
{
public:
    /// Parses a file and writes the result into `geometryData`.
    static bool parseFile(const QString &filePath, GeometryData &geometryData, QString *errorMessage = nullptr);
};

} // namespace PolyShow
