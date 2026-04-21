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
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#2B2B2B")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#E4E4E4")));
    palette.setColor(QPalette::Base, QColor(QStringLiteral("#353535")));
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#2F2F2F")));
    palette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#3B3B3B")));
    palette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#F2F2F2")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#E4E4E4")));
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#353535")));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#E4E4E4")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#3D7BFF")));
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    palette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#9A9A9A")));
    palette.setColor(QPalette::BrightText, QColor(QStringLiteral("#FFFFFF")));
    return palette;
}

QString DarkStyle::styleSheet()
{
    return loadStyleSheet(QStringLiteral(":/style/darkstyle.qss"));
}

} // namespace PolyShow
