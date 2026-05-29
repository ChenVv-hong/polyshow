#include "ui/MaterialIcon.h"

#include <QFont>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QtGlobal>

#include <cmath>

namespace PolyShow
{

namespace
{

void appendUniqueSize(QList<int> &sizes, int size)
{
    const int normalizedSize = qMax(size, 1);
    if (!sizes.contains(normalizedSize))
    {
        sizes.append(normalizedSize);
    }
}

QPixmap renderIconPixmap(const QString &iconName, const QColor &color, int logicalSize, qreal devicePixelRatio)
{
    const int physicalSize = qMax(1, static_cast<int>(std::ceil(logicalSize * devicePixelRatio)));
    QPixmap pixmap(physicalSize, physicalSize);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QFont iconFont(MaterialIcon::fontFamily());
    iconFont.setPixelSize(logicalSize);
    iconFont.setWeight(QFont::Normal);
    iconFont.setStyleStrategy(QFont::PreferAntialias);

    painter.setFont(iconFont);
    painter.setPen(color);
    painter.drawText(QRectF(0, 0, logicalSize, logicalSize), Qt::AlignCenter, iconName);

    return pixmap;
}

} // namespace

QString MaterialIcon::fontFamily()
{
    return QStringLiteral("Material Symbols Rounded");
}

QIcon MaterialIcon::icon(const QString &iconName, const QColor &color, int size)
{
    const int normalizedSize = qMax(size, 1);

    QList<int> logicalSizes;
    appendUniqueSize(logicalSizes, normalizedSize);
    appendUniqueSize(logicalSizes, qMax(normalizedSize, 24));
    appendUniqueSize(logicalSizes, qMax(normalizedSize, 32));
    appendUniqueSize(logicalSizes, qMax(normalizedSize, 48));

    QIcon icon;
    const qreal devicePixelRatios[] = {1.0, 1.5, 2.0, 3.0};
    for (int logicalSize : logicalSizes)
    {
        for (qreal devicePixelRatio : devicePixelRatios)
        {
            icon.addPixmap(renderIconPixmap(iconName, color, logicalSize, devicePixelRatio));
        }
    }

    return icon;
}

} // namespace PolyShow
