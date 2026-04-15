#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

namespace PolyShow
{

/// Supported UI theme modes.
enum class ThemeMode
{
    Light,
    Dark
};

/// Shared color tokens used by the UI.
struct ThemeColors
{
    QColor window_background;
    QColor panel_background;
    QColor card_background;
    QColor canvas_background;
    QColor border_subtle;
    QColor border_focus;
    QColor text_primary;
    QColor text_secondary;
    QColor text_muted;
    QColor accent_primary;
    QColor accent_primary_soft;
    QColor accent_success;
    QColor accent_success_soft;
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

/// Centralized UI theme helpers.
class UiTheme final
{
public:
    /// Returns the colors for one theme mode.
    [[nodiscard]]
    static const ThemeColors &colors(ThemeMode themeMode);

    /// Builds an application palette for one theme mode.
    [[nodiscard]]
    static QPalette palette(ThemeMode themeMode);

    /// Builds the shared application stylesheet for one theme mode.
    [[nodiscard]]
    static QString styleSheet(ThemeMode themeMode);
};

} // namespace PolyShow
