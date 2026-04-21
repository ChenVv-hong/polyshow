#include "style/LightStyle.h"

#include <QFile>
#include <QStyleFactory>

namespace PolyShow
{

namespace
{

/// Loads one stylesheet resource into memory.
QString loadStyleSheet(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }

    return QString::fromUtf8(file.readAll());
}

} // namespace

LightStyle::LightStyle()
    : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
{
}

QPalette LightStyle::palette()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#F5F5F5")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#202020")));
    palette.setColor(QPalette::Base, QColor(QStringLiteral("#FFFFFF")));
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#F7F7F7")));
    palette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#FFFFFF")));
    palette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#202020")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#202020")));
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#EFEFEF")));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#202020")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#2D74FF")));
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    palette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#858585")));
    palette.setColor(QPalette::BrightText, QColor(QStringLiteral("#FFFFFF")));
    return palette;
}

QString LightStyle::styleSheet()
{
    return loadStyleSheet(QStringLiteral(":/style/lightstyle.qss"));
}

} // namespace PolyShow
