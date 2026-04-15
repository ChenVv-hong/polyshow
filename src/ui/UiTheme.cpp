#include "ui/UiTheme.h"

#include <QStringList>

namespace PolyShow
{

namespace
{

ThemeColors makeLightColors()
{
    return ThemeColors {
        QColor(QStringLiteral("#F8FAFC")),
        QColor(QStringLiteral("#FFFFFF")),
        QColor(QStringLiteral("#F8FAFC")),
        QColor(QStringLiteral("#F8FAFC")),
        QColor(QStringLiteral("#E2E8F0")),
        QColor(QStringLiteral("#93C5FD")),
        QColor(QStringLiteral("#0F172A")),
        QColor(QStringLiteral("#334155")),
        QColor(QStringLiteral("#64748B")),
        QColor(QStringLiteral("#2563EB")),
        QColor(QStringLiteral("#DBEAFE")),
        QColor(QStringLiteral("#059669")),
        QColor(QStringLiteral("#ECFDF5")),
        QColor(QStringLiteral("#E2E8F0")),
        QColor(QStringLiteral("#94A3B8")),
        QColor(QStringLiteral("#3B82F6")),
        QColor(QStringLiteral("#2563EB1A")),
        QColor(QStringLiteral("#991B1B")),
        QColor(QStringLiteral("#FECACA")),
        QColor(QStringLiteral("#92400E")),
        QColor(QStringLiteral("#FDE68A")),
        QColor(QStringLiteral("#1E3A8A")),
        QColor(QStringLiteral("#BFDBFE"))
    };
}

ThemeColors makeDarkColors()
{
    return ThemeColors {
        QColor(QStringLiteral("#0F172A")),
        QColor(QStringLiteral("#111827")),
        QColor(QStringLiteral("#1F2937")),
        QColor(QStringLiteral("#111827")),
        QColor(QStringLiteral("#334155")),
        QColor(QStringLiteral("#60A5FA")),
        QColor(QStringLiteral("#F8FAFC")),
        QColor(QStringLiteral("#CBD5E1")),
        QColor(QStringLiteral("#94A3B8")),
        QColor(QStringLiteral("#60A5FA")),
        QColor(QStringLiteral("#1D4ED8")),
        QColor(QStringLiteral("#34D399")),
        QColor(QStringLiteral("#064E3B")),
        QColor(QStringLiteral("#1E293B")),
        QColor(QStringLiteral("#475569")),
        QColor(QStringLiteral("#60A5FA")),
        QColor(QStringLiteral("#1D4ED833")),
        QColor(QStringLiteral("#FCA5A5")),
        QColor(QStringLiteral("#7F1D1D")),
        QColor(QStringLiteral("#FCD34D")),
        QColor(QStringLiteral("#78350F")),
        QColor(QStringLiteral("#93C5FD")),
        QColor(QStringLiteral("#1E3A8A"))
    };
}

QString colorHex(const QColor &color)
{
    return color.name(QColor::HexArgb);
}

} // namespace

const ThemeColors &UiTheme::colors(ThemeMode themeMode)
{
    static const ThemeColors kLightColors = makeLightColors();
    static const ThemeColors kDarkColors = makeDarkColors();
    return themeMode == ThemeMode::Dark ? kDarkColors : kLightColors;
}

QPalette UiTheme::palette(ThemeMode themeMode)
{
    const ThemeColors &themeColors = colors(themeMode);

    QPalette palette;
    palette.setColor(QPalette::Window, themeColors.window_background);
    palette.setColor(QPalette::WindowText, themeColors.text_primary);
    palette.setColor(QPalette::Base, themeColors.panel_background);
    palette.setColor(QPalette::AlternateBase, themeColors.card_background);
    palette.setColor(QPalette::ToolTipBase, themeColors.panel_background);
    palette.setColor(QPalette::ToolTipText, themeColors.text_primary);
    palette.setColor(QPalette::Text, themeColors.text_primary);
    palette.setColor(QPalette::Button, themeColors.panel_background);
    palette.setColor(QPalette::ButtonText, themeColors.text_primary);
    palette.setColor(QPalette::BrightText, themeColors.panel_background);
    palette.setColor(QPalette::Highlight, themeColors.accent_primary);
    palette.setColor(QPalette::HighlightedText, QColor(Qt::white));
    palette.setColor(QPalette::PlaceholderText, themeColors.text_muted);
    return palette;
}

QString UiTheme::styleSheet(ThemeMode themeMode)
{
    const ThemeColors &c = colors(themeMode);

    return QStringLiteral(
               "QWidget {"
               "  color: %1;"
               "  font-family: 'Inter';"
               "  font-size: 12px;"
               "}"
               "QMainWindow {"
               "  background: %2;"
               "}"
               "PanelFrame[panelVariant=\"panel\"],"
               "PanelFrame[panelVariant=\"card\"],"
               "PanelFrame[panelVariant=\"canvas\"] {"
               "  border: 1px solid %3;"
               "  border-radius: 12px;"
               "  background: %4;"
               "}"
               "PanelFrame[panelVariant=\"card\"] {"
               "  background: %5;"
               "  border-radius: 10px;"
               "}"
               "PanelFrame[panelVariant=\"canvas\"] {"
               "  background: %6;"
               "  border-radius: 12px;"
               "}"
               "PillButton {"
               "  border-radius: 14px;"
               "  border: 1px solid %3;"
               "  padding: 6px 10px;"
               "  background: %5;"
               "  color: %7;"
               "}"
               "PillButton:hover {"
               "  border-color: %8;"
               "}"
               "PillButton[buttonVariant=\"primary\"] {"
               "  background: %9;"
               "  color: %8;"
               "  border-color: %8;"
               "}"
               "PillButton[buttonVariant=\"success\"] {"
               "  background: %10;"
               "  color: %11;"
               "  border-color: %11;"
               "}"
               "QTreeWidget {"
               "  background: %5;"
               "  border: 1px solid %3;"
               "  border-radius: 10px;"
               "  outline: 0;"
               "}"
               "QTreeWidget::item {"
               "  padding: 4px 0px;"
               "}"
               "QTreeWidget::item:selected {"
               "  background: %9;"
               "  color: %8;"
               "}"
               "QTreeWidget::branch:selected {"
               "  background: transparent;"
               "}"
               "QLineEdit {"
               "  background: %4;"
               "  border: 1px solid %3;"
               "  border-radius: 10px;"
               "  padding: 6px 10px;"
               "}"
               "QLineEdit:focus {"
               "  border-color: %8;"
               "}"
               "QPushButton {"
               "  background: %4;"
               "  color: %1;"
               "  border: 1px solid %3;"
               "  border-radius: 8px;"
               "  padding: 6px 10px;"
               "}"
               "QPushButton:checked {"
               "  background: %9;"
               "  color: %8;"
               "  border-color: %8;"
               "}"
               "QLabel[role=\"sectionTitle\"] {"
               "  color: %12;"
               "  font-size: 11px;"
               "  font-weight: 600;"
               "}"
               "QLabel[role=\"panelTitle\"] {"
               "  color: %1;"
               "  font-size: 15px;"
               "  font-weight: 700;"
               "}"
               "QLabel#inspectorBadge {"
               "  background: %9;"
               "  color: %8;"
               "  border-radius: 10px;"
               "  padding: 3px 8px;"
               "  font-size: 11px;"
               "  font-weight: 600;"
               "}"
               "QLabel[role=\"mono\"] {"
                "  font-family: 'IBM Plex Mono';"
                "}"
               "QListWidget {"
               "  background: %5;"
               "  border: none;"
               "}"
               "QListWidget::item {"
               "  border-radius: 8px;"
               "  padding: 5px 8px;"
               "  margin: 1px 0px;"
               "}"
               "QCheckBox {"
               "  spacing: 8px;"
               "}"
               "QCheckBox::indicator {"
               "  width: 14px;"
               "  height: 14px;"
               "  border-radius: 4px;"
               "  border: 1px solid %12;"
               "  background: %4;"
               "}"
               "QCheckBox::indicator:checked {"
               "  border-color: %8;"
               "  background: %8;"
               "}"
               "QComboBox {"
               "  background: %5;"
               "  border: 1px solid %3;"
               "  border-radius: 14px;"
               "  padding: 4px 10px;"
               "}"
               "QStatusBar {"
               "  background: %4;"
               "}"
               "QMenuBar {"
               "  background: %4;"
               "}"
               "QMenu {"
               "  background: %4;"
               "  border: 1px solid %3;"
               "}")
        .arg(colorHex(c.text_primary))
        .arg(colorHex(c.window_background))
        .arg(colorHex(c.border_subtle))
        .arg(colorHex(c.panel_background))
        .arg(colorHex(c.card_background))
        .arg(colorHex(c.canvas_background))
        .arg(colorHex(c.text_secondary))
        .arg(colorHex(c.accent_primary))
        .arg(colorHex(c.accent_primary_soft))
        .arg(colorHex(c.accent_success_soft))
        .arg(colorHex(c.accent_success))
        .arg(colorHex(c.text_muted));
}

} // namespace PolyShow
