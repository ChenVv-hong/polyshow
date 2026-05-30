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
    QColor overlay_panel_background;
    QColor overlay_panel_border;
    QColor overlay_title_text;
    QColor overlay_body_text;
    QColor overlay_muted_text;
    QColor log_error_text;
    QColor log_error_border;
    QColor log_warning_text;
    QColor log_warning_border;
    QColor log_info_text;
    QColor log_info_border;
    QColor outliner_row_text;
    QColor outliner_row_selected_text;
    QColor outliner_row_hidden_text;
    QColor outliner_row_background;
    QColor outliner_row_hover_background;
    QColor outliner_row_selected_background;
    QColor outliner_icon;
    QColor outliner_icon_selected;
    QColor outliner_check_border;
    QColor outliner_check_checked_border;
    QColor outliner_check_background;
    QColor outliner_check_checked_background;
    QColor outliner_check_mark;
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
