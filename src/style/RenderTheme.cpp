#include "style/RenderTheme.h"

namespace PolyShow
{

namespace
{

ThemeMode g_active_theme = ThemeMode::Light;

RenderColors makeLightColors()
{
    return RenderColors {
        QColor(QStringLiteral("#F8FAFC")),
        QColor(QStringLiteral("#E1E7EF")),
        QColor(QStringLiteral("#AAB7C6")),
        QColor(QStringLiteral("#2D74FF")),
        QColor(QStringLiteral("#2D74FF22")),
        QColor(QStringLiteral("#F4F6F9EE")),
        QColor(QStringLiteral("#D3DAE3")),
        QColor(QStringLiteral("#20242A")),
        QColor(QStringLiteral("#394453")),
        QColor(QStringLiteral("#5E6875")),
        QColor(QStringLiteral("#B24A4A")),
        QColor(QStringLiteral("#FCE7E7")),
        QColor(QStringLiteral("#8A5A14")),
        QColor(QStringLiteral("#FFF4D8")),
        QColor(QStringLiteral("#15408F")),
        QColor(QStringLiteral("#E3F0FF"))
    };
}

RenderColors makeDarkColors()
{
    return RenderColors {
        QColor(QStringLiteral("#1F2228")),
        QColor(QStringLiteral("#343943")),
        QColor(QStringLiteral("#556070")),
        QColor(QStringLiteral("#63A7FF")),
        QColor(QStringLiteral("#63A7FF28")),
        QColor(QStringLiteral("#303746EE")),
        QColor(QStringLiteral("#45484F")),
        QColor(QStringLiteral("#E9EAEC")),
        QColor(QStringLiteral("#AEB4BC")),
        QColor(QStringLiteral("#7E858F")),
        QColor(QStringLiteral("#ED8A8A")),
        QColor(QStringLiteral("#4A2A2A")),
        QColor(QStringLiteral("#E7BF67")),
        QColor(QStringLiteral("#4A3C22")),
        QColor(QStringLiteral("#63A7FF")),
        QColor(QStringLiteral("#25384E"))
    };
}

} // namespace

const RenderColors &RenderTheme::colors(ThemeMode themeMode)
{
    static const RenderColors kLightColors = makeLightColors();
    static const RenderColors kDarkColors = makeDarkColors();
    return themeMode == ThemeMode::Dark ? kDarkColors : kLightColors;
}

const RenderColors &RenderTheme::colors()
{
    return colors(g_active_theme);
}

void RenderTheme::setActiveTheme(ThemeMode themeMode)
{
    g_active_theme = themeMode;
}

ThemeMode RenderTheme::activeTheme()
{
    return g_active_theme;
}

} // namespace PolyShow
