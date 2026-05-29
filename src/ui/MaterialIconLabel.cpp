#include "ui/MaterialIconLabel.h"

#include <QStyle>
#include <QtGlobal>

namespace PolyShow
{

MaterialIconLabel::MaterialIconLabel(const QString &iconName, QWidget *parent)
    : QLabel(parent)
{
    setObjectName(QStringLiteral("materialIcon"));
    setAlignment(Qt::AlignCenter);
    setIconPixelSize(16);
    setIconName(iconName);
}

void MaterialIconLabel::setIconName(const QString &iconName)
{
    setText(iconName);
}

void MaterialIconLabel::setIconPixelSize(int pixelSize)
{
    const int normalizedPixelSize = qMax(pixelSize, 1);
    setProperty("iconSize", QString::number(normalizedPixelSize));
    setFixedSize(normalizedPixelSize, normalizedPixelSize);

    style()->unpolish(this);
    style()->polish(this);
    update();
}

} // namespace PolyShow
