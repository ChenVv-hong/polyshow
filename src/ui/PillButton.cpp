#include "ui/PillButton.h"

#include <QStyle>

namespace PolyShow
{

PillButton::PillButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    setCursor(Qt::PointingHandCursor);
    setVariant(Variant::Neutral);
}

void PillButton::setVariant(Variant variant)
{
    setProperty("buttonVariant", variantName(variant));
    style()->unpolish(this);
    style()->polish(this);
}

QString PillButton::variantName(Variant variant)
{
    switch (variant)
    {
    case Variant::Neutral:
        return QStringLiteral("neutral");
    case Variant::Primary:
        return QStringLiteral("primary");
    case Variant::Success:
        return QStringLiteral("success");
    default:
        return QStringLiteral("neutral");
    }
}

} // namespace PolyShow
