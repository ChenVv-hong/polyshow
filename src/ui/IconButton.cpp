#include "ui/IconButton.h"

#include "ui/MaterialIcon.h"

#include <QSize>

namespace PolyShow
{

IconButton::IconButton(const QString &iconName, const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    setObjectName(text.isEmpty() ? QStringLiteral("iconButton") : QStringLiteral("toolButton"));
    setCursor(Qt::PointingHandCursor);
    setIconSize(QSize(16, 16));
    setIconName(iconName);
}

void IconButton::setIconName(const QString &iconName)
{
    setProperty("materialIcon", iconName);
    setIcon(MaterialIcon::icon(iconName));
}

} // namespace PolyShow
