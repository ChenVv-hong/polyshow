#include "ui/PanelFrame.h"

#include "ui/UiTheme.h"

namespace PolyShow
{

PanelFrame::PanelFrame(Variant variant, QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
    setAutoFillBackground(true);
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName(QStringLiteral("PanelFrameWidget"));
    setVariant(variant);
}

void PanelFrame::setVariant(Variant variant)
{
    setProperty("panelVariant", variantName(variant));
    applyVariantStyle(variant);
}

QString PanelFrame::variantName(Variant variant)
{
    switch (variant)
    {
    case Variant::Panel:
        return QStringLiteral("panel");
    case Variant::Card:
        return QStringLiteral("card");
    case Variant::Canvas:
        return QStringLiteral("canvas");
    default:
        return QStringLiteral("panel");
    }
}

void PanelFrame::applyVariantStyle(Variant variant)
{
    const ThemeColors &colors = UiTheme::colors(ThemeMode::Light);

    QString backgroundColor = colors.panel_background.name(QColor::HexArgb);
    int borderRadius = 12;

    switch (variant)
    {
    case Variant::Panel:
        backgroundColor = colors.panel_background.name(QColor::HexArgb);
        borderRadius = 12;
        break;
    case Variant::Card:
        backgroundColor = colors.card_background.name(QColor::HexArgb);
        borderRadius = 10;
        break;
    case Variant::Canvas:
        backgroundColor = colors.panel_background.name(QColor::HexArgb);
        borderRadius = 12;
        break;
    default:
        break;
    }

    // setStyleSheet(QStringLiteral(
    //     "#PanelFrameWidget {"
    //     "  background-color: %1;"
    //     "  border: 1px solid %2;"
    //     "  border-radius: %3px;"
    //     "}").arg(backgroundColor, colors.border_subtle.name(QColor::HexArgb), QString::number(borderRadius)));
}

} // namespace PolyShow
