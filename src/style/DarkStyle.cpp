#include "style/DarkStyle.h"

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

DarkStyle::DarkStyle()
    : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
{
}

QPalette DarkStyle::palette()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#202124")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#E9EAEC")));
    palette.setColor(QPalette::Base, QColor(QStringLiteral("#303236")));
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#282A2E")));
    palette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#303236")));
    palette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#E9EAEC")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#E9EAEC")));
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#282A2E")));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#E9EAEC")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#63A7FF")));
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    palette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#7E858F")));
    palette.setColor(QPalette::BrightText, QColor(QStringLiteral("#FFFFFF")));
    return palette;
}

QString DarkStyle::styleSheet()
{
    return loadStyleSheet(QStringLiteral(":/style/darkstyle.qss"));
}

} // namespace PolyShow
