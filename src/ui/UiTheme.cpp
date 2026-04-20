#include "ui/UiTheme.h"

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
               "  background-color: %2;"
               "}"
               "QWidget#workspaceRoot {"
               "  background-color: %2;"
               "}"
               "PanelFrame[panelVariant=\"panel\"] {"
                "  background-color: %3;"
                "  border: 1px solid %4;"
                "  border-radius: 12px;"
                "}"
               "PanelFrame[panelVariant=\"card\"] {"
               "  background-color: %5;"
               "  border: 1px solid %4;"
               "  border-radius: 10px;"
               "}"
               "PanelFrame[panelVariant=\"canvas\"] {"
               "  background-color: %3;"
               "  border: 1px solid %4;"
               "  border-radius: 12px;"
               "}"
               "PillButton {"
               "  border-radius: 14px;"
               "  border: 1px solid %4;"
               "  padding: 6px 10px;"
               "  background-color: %5;"
               "  color: %6;"
               "}"
               "PillButton:hover {"
               "  border-color: %7;"
               "}"
               "PillButton[buttonVariant=\"primary\"] {"
               "  background-color: %8;"
               "  color: %7;"
               "  border-color: %7;"
               "}"
               "PillButton[buttonVariant=\"success\"] {"
               "  background-color: %9;"
               "  color: %10;"
               "  border-color: %10;"
               "}"
               "QTreeWidget {"
               "  background-color: %5;"
               "  border: 1px solid %4;"
               "  border-radius: 10px;"
               "  outline: 0;"
               "}"
               "QTreeWidget::item {"
               "  padding: 4px 0px;"
               "}"
               "QTreeWidget::item:selected {"
               "  background-color: %8;"
               "  color: %7;"
               "}"
               "QTreeWidget::branch:selected {"
               "  background-color: transparent;"
               "}"
               "QLineEdit {"
               "  background-color: %3;"
               "  border: 1px solid %4;"
               "  border-radius: 10px;"
               "  padding: 6px 10px;"
               "}"
               "QLineEdit:focus {"
               "  border-color: %7;"
               "}"
               "QLineEdit[validationState=\"error\"] {"
               "  border-color: %13;"
               "}"
               "QPlainTextEdit {"
               "  background-color: %3;"
               "  border: 1px solid %4;"
               "  border-radius: 10px;"
               "  padding: 6px 10px;"
               "}"
               "QPlainTextEdit:focus {"
               "  border-color: %7;"
               "}"
               "QPlainTextEdit[validationState=\"error\"] {"
               "  border-color: %13;"
               "}"
               "QPushButton {"
               "  background-color: %5;"
               "  color: %1;"
               "  border: 1px solid %4;"
               "  border-radius: 8px;"
               "  padding: 6px 10px;"
               "}"
               "QPushButton:checked {"
               "  background-color: %8;"
               "  color: %7;"
               "  border-color: %7;"
               "}"
               "QLabel[role=\"sectionTitle\"] {"
               "  color: %11;"
               "  font-size: 11px;"
               "  font-weight: 600;"
               "}"
               "QLabel[role=\"panelTitle\"] {"
               "  color: %1;"
               "  font-size: 15px;"
               "  font-weight: 700;"
               "}"
               "QLabel#inspectorBadge {"
               "  background-color: %8;"
               "  color: %7;"
               "  border-radius: 10px;"
               "  padding: 3px 8px;"
               "  font-size: 11px;"
               "  font-weight: 600;"
               "}"
               "QLabel[role=\"mono\"] {"
               "  font-family: 'IBM Plex Mono';"
               "}"
               "QLabel[role=\"validationError\"] {"
               "  color: %14;"
               "  font-size: 11px;"
               "}"
               "QListWidget {"
               "  background-color: %5;"
               "  border: none;"
               "}"
               "QListWidget::item {"
               "  border-radius: 8px;"
               "  padding: 5px 8px;"
               "  margin: 1px 0px;"
               "}"
               "QTreeView::indicator, QCheckBox::indicator {"
               "  width: 14px;"
               "  height: 14px;"
               "  border-radius: 4px;"
               "  border: 1px solid %11;"
               "  background-color: %3;"
               "}"
               "QTreeView::indicator:checked, QCheckBox::indicator:checked {"
               "  border-color: %7;"
               "  background-color: %7;"
               "}"
               "QScrollBar:vertical {"
               "  background-color: transparent;"
               "  width: 10px;"
               "  margin: 2px 0px 2px 0px;"
               "}"
               "QScrollBar:horizontal {"
               "  background-color: transparent;"
               "  height: 10px;"
               "  margin: 0px 2px 0px 2px;"
               "}"
               "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
               "  background-color: %11;"
               "  border-radius: 5px;"
               "  min-height: 24px;"
               "  min-width: 24px;"
               "}"
               "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {"
               "  background-color: %6;"
               "}"
               "QScrollBar::add-line, QScrollBar::sub-line, QScrollBar::add-page, QScrollBar::sub-page {"
               "  background-color: transparent;"
               "  border: none;"
               "}"
               "QSplitter::handle {"
                "  background-color: %4;"
                "}"
                "QSplitter::handle:hover {"
               "  background-color: %7;"
                "}"
               "QSplitter::handle:horizontal {"
                "  width: 6px;"
                "}"
                "QSplitter::handle:vertical {"
                "  height: 6px;"
                "}"
               "QSplitter::handle:pressed {"
               "  background-color: %7;"
               "}"
               "QComboBox {"
               "  background-color: %5;"
               "  border: 1px solid %4;"
               "  border-radius: 14px;"
               "  padding: 4px 10px;"
               "}"
               "QStatusBar {"
               "  background-color: %3;"
               "}"
               "QMenuBar {"
               "  background-color: %3;"
               "}"
               "QGraphicsView {"
               "  background-color: %12;"
               "  border: 1px solid %4;"
               "  border-radius: 10px;"
               "}"
               "QMenu {"
               "  background-color: %3;"
               "  border: 1px solid %4;"
               "}")
        .arg(colorHex(c.text_primary))
        .arg(colorHex(c.window_background))
        .arg(colorHex(c.panel_background))
        .arg(colorHex(c.border_subtle))
        .arg(colorHex(c.card_background))
        .arg(colorHex(c.text_secondary))
        .arg(colorHex(c.accent_primary))
        .arg(colorHex(c.accent_primary_soft))
        .arg(colorHex(c.accent_success_soft))
        .arg(colorHex(c.accent_success))
        .arg(colorHex(c.text_muted))
        .arg(colorHex(c.canvas_background))
        .arg(colorHex(c.log_error_border))
        .arg(colorHex(c.log_error_text));
}

} // namespace PolyShow
