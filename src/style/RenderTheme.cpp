#include "style/RenderTheme.h"

namespace PolyShow
{

namespace
{

ThemeMode g_active_theme = ThemeMode::Light;

RenderColors makeLightColors()
{
    return RenderColors {
        QColor(QStringLiteral("#FFFFFF")),
        QColor(QStringLiteral("#E6E6E6")),
        QColor(QStringLiteral("#B7B7B7")),
        QColor(QStringLiteral("#3D7BFF")),
        QColor(QStringLiteral("#3D7BFF22")),
        QColor(QStringLiteral("#F8FAFC")),
        QColor(QStringLiteral("#D7DEE8")),
        QColor(QStringLiteral("#1F2937")),
        QColor(QStringLiteral("#334155")),
        QColor(QStringLiteral("#64748B")),
        QColor(QStringLiteral("#8B1E1E")),
        QColor(QStringLiteral("#E2A0A0")),
        QColor(QStringLiteral("#8A5A14")),
        QColor(QStringLiteral("#F1D28A")),
        QColor(QStringLiteral("#1D4E89")),
        QColor(QStringLiteral("#A8C9F5"))
    };
}

RenderColors makeDarkColors()
{
    return RenderColors {
        QColor(QStringLiteral("#2B2B2B")),
        QColor(QStringLiteral("#3A3A3A")),
        QColor(QStringLiteral("#505050")),
        QColor(QStringLiteral("#4E8CFF")),
        QColor(QStringLiteral("#4E8CFF33")),
        QColor(QStringLiteral("#303746")),
        QColor(QStringLiteral("#556070")),
        QColor(QStringLiteral("#F3F6FB")),
        QColor(QStringLiteral("#D8E2F0")),
        QColor(QStringLiteral("#AAB9CC")),
        QColor(QStringLiteral("#F3A0A0")),
        QColor(QStringLiteral("#7A2C2C")),
        QColor(QStringLiteral("#E7C170")),
        QColor(QStringLiteral("#6E5520")),
        QColor(QStringLiteral("#A9D1FF")),
        QColor(QStringLiteral("#254A73"))
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
