#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

namespace PolyShow
{

/// Shared helper for rendering Google Material Symbols Rounded ligature icons.
class MaterialIcon final
{
public:
    /// Returns the font family used by Material Symbols Rounded.
    [[nodiscard]]
    static QString fontFamily();

    /// Creates a Qt icon pixmap from one Material Symbols ligature name.
    [[nodiscard]]
    static QIcon icon(const QString &iconName, const QColor &color = QColor(QStringLiteral("#AEB4BC")), int size = 18);
};

} // namespace PolyShow
