#include "ui/PillButton.h"

#include "ui/MaterialIcon.h"

#include <QSize>
#include <QStyle>

namespace PolyShow
{

PillButton::PillButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    setObjectName(QStringLiteral("toolButton"));
    setCursor(Qt::PointingHandCursor);
    setIconSize(QSize(16, 16));
    setVariant(Variant::Neutral);
}

void PillButton::setVariant(Variant variant)
{
    setProperty("buttonVariant", variantName(variant));
    style()->unpolish(this);
    style()->polish(this);
}

void PillButton::setIconName(const QString &iconName)
{
    setProperty("materialIcon", iconName);
    setIcon(MaterialIcon::icon(iconName));
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
