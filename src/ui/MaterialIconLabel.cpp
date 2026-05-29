#include "ui/MaterialIconLabel.h"

#include "ui/MaterialIcon.h"

#include <QFont>

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
    QFont iconFont(MaterialIcon::fontFamily());
    iconFont.setPixelSize(pixelSize);
    iconFont.setWeight(QFont::Normal);
    setFont(iconFont);
    setFixedSize(pixelSize, pixelSize);
}

} // namespace PolyShow
