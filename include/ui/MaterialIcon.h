#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

namespace PolyShow
{

/// Shared helper for producing Qt-native icons from Material Symbols Rounded ligatures.
class MaterialIcon final
{
public:
    static constexpr int kDefaultIconSize = 24;

    /// Returns the font family used by Material Symbols Rounded.
    [[nodiscard]]
    static QString fontFamily();

    /// Creates a multi-resolution Qt icon from one Material Symbols ligature name.
    [[nodiscard]]
    static QIcon icon(
        const QString &iconName,
        const QColor &color = QColor(QStringLiteral("#AEB4BC")),
        int size = kDefaultIconSize);
};

} // namespace PolyShow
