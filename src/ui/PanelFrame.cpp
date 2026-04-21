#include "ui/PanelFrame.h"

#include <QStyle>

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
    style()->unpolish(this);
    style()->polish(this);
    update();
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

} // namespace PolyShow
