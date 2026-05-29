#include "ui/MaterialIcon.h"

#include <QFont>
#include <QPainter>
#include <QPixmap>
#include <QtGlobal>

namespace PolyShow
{

QString MaterialIcon::fontFamily()
{
    return QStringLiteral("Material Symbols Rounded");
}

QIcon MaterialIcon::icon(const QString &iconName, const QColor &color, int size)
{
    const int pixmapSize = qMax(size, 1);
    QPixmap pixmap(pixmapSize, pixmapSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QFont iconFont(MaterialIcon::fontFamily());
    iconFont.setPixelSize(size);
    iconFont.setWeight(QFont::Normal);
    painter.setFont(iconFont);
    painter.setPen(color);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, iconName);

    return QIcon(pixmap);
}

} // namespace PolyShow
