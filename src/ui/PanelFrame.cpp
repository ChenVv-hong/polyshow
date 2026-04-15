#include "ui/PanelFrame.h"

#include <QStyle>

namespace PolyShow
{

PanelFrame::PanelFrame(Variant variant, QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
    setVariant(variant);
}

void PanelFrame::setVariant(Variant variant)
{
    setProperty("panelVariant", variantName(variant));
    style()->unpolish(this);
    style()->polish(this);
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
