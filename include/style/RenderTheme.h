#pragma once

#include "style/ThemeMode.h"

#include <QColor>

namespace PolyShow
{

/// Runtime drawing colors used outside the Qt widget stylesheet system.
struct RenderColors
{
    QColor canvas_background;
    QColor grid_line;
    QColor axis_line;
    QColor selection_stroke;
    QColor selection_fill;
    QColor log_error_text;
    QColor log_error_border;
    QColor log_warning_text;
    QColor log_warning_border;
    QColor log_info_text;
    QColor log_info_border;
};

/// Centralized runtime drawing theme helpers.
class RenderTheme final
{
public:
    /// Returns drawing colors for the requested theme mode.
    [[nodiscard]]
    static const RenderColors &colors(ThemeMode themeMode);

    /// Returns drawing colors for the currently active application theme.
    [[nodiscard]]
    static const RenderColors &colors();

    /// Updates the currently active application theme.
    static void setActiveTheme(ThemeMode themeMode);

    /// Returns the currently active application theme.
    [[nodiscard]]
    static ThemeMode activeTheme();
};

} // namespace PolyShow
