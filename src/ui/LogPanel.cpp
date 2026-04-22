#include "ui/LogPanel.h"

#include "style/RenderTheme.h"

#include <QFont>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

namespace PolyShow
{

namespace
{

QColor lineBorder(LogSeverity severity)
{
    const RenderColors &renderColors = RenderTheme::colors();
    switch (severity)
    {
    case LogSeverity::Error:
        return renderColors.log_error_border;
    case LogSeverity::Warning:
        return renderColors.log_warning_border;
    case LogSeverity::Info:
        return renderColors.log_info_border;
    default:
        return QColor(QStringLiteral("#808080"));
    }
}

QColor lineText(LogSeverity severity)
{
    const RenderColors &renderColors = RenderTheme::colors();
    switch (severity)
    {
    case LogSeverity::Error:
        return renderColors.log_error_text;
    case LogSeverity::Warning:
        return renderColors.log_warning_text;
    case LogSeverity::Info:
        return renderColors.log_info_text;
    default:
        return QColor(QStringLiteral("#808080"));
    }
}

} // namespace

LogPanel::LogPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_list_widget = new QListWidget(this);
    m_list_widget->setSelectionMode(QAbstractItemView::NoSelection);
    m_list_widget->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_list_widget);
}

void LogPanel::appendMessage(LogSeverity severity, const QString &message)
{
    auto *item = new QListWidgetItem(message, m_list_widget);
    item->setFont(QFont(QStringLiteral("IBM Plex Mono"), 10));
    item->setForeground(lineText(severity));
    item->setBackground(m_list_widget->palette().base());
    item->setData(Qt::UserRole, lineBorder(severity));
    item->setToolTip(message);
}

void LogPanel::clearMessages()
{
    m_list_widget->clear();
}

} // namespace PolyShow
