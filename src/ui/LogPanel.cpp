#include "ui/LogPanel.h"

#include "ui/EditorPanelHeader.h"
#include "ui/MaterialIconLabel.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>

namespace PolyShow
{

namespace
{

QString severityName(LogSeverity severity)
{
    switch (severity)
    {
    case LogSeverity::Error:
        return QStringLiteral("error");
    case LogSeverity::Warning:
        return QStringLiteral("warning");
    case LogSeverity::Info:
        return QStringLiteral("info");
    default:
        return QStringLiteral("info");
    }
}

QString severityIcon(LogSeverity severity)
{
    switch (severity)
    {
    case LogSeverity::Error:
        return QStringLiteral("error");
    case LogSeverity::Warning:
        return QStringLiteral("warning");
    case LogSeverity::Info:
        return QStringLiteral("info");
    default:
        return QStringLiteral("info");
    }
}

QWidget *createLogRow(LogSeverity severity, const QString &message, QWidget *parent)
{
    auto *row = new QWidget(parent);
    row->setObjectName(QStringLiteral("logRow"));
    row->setProperty("severity", severityName(severity));
    row->setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(7);

    auto *icon = new MaterialIconLabel(severityIcon(severity), row);
    icon->setProperty("iconRole", QStringLiteral("log"));
    icon->setProperty("severity", severityName(severity));
    icon->setIconPixelSize(16);
    layout->addWidget(icon);

    auto *label = new QLabel(message, row);
    label->setObjectName(QStringLiteral("logMessage"));
    label->setProperty("severity", severityName(severity));
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(false);
    layout->addWidget(label, 1);

    return row;
}

} // namespace

LogPanel::LogPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    setObjectName(QStringLiteral("logPanel"));
    setAttribute(Qt::WA_StyledBackground, true);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new EditorPanelHeader(QStringLiteral("terminal"), QStringLiteral("Log"), this);
    layout->addWidget(header);

    m_list_widget = new QListWidget(this);
    m_list_widget->setObjectName(QStringLiteral("logList"));
    m_list_widget->setSelectionMode(QAbstractItemView::NoSelection);
    m_list_widget->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_list_widget);
}

void LogPanel::appendMessage(LogSeverity severity, const QString &message)
{
    auto *item = new QListWidgetItem(m_list_widget);
    item->setSizeHint(QSize(0, 24));
    item->setToolTip(message);

    m_list_widget->setItemWidget(item, createLogRow(severity, message, m_list_widget));
}

void LogPanel::clearMessages()
{
    m_list_widget->clear();
}

} // namespace PolyShow
