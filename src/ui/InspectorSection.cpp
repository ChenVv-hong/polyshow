#include "ui/InspectorSection.h"

#include "ui/MaterialIconLabel.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>
#include <QVBoxLayout>

namespace PolyShow
{

InspectorSection::InspectorSection(const QString &title, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("inspectorSection"));
    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_header_widget = new QWidget(this);
    m_header_widget->setObjectName(QStringLiteral("inspectorSectionHeader"));
    m_header_widget->setAttribute(Qt::WA_StyledBackground, true);
    m_header_widget->setCursor(Qt::PointingHandCursor);
    m_header_widget->setFocusPolicy(Qt::StrongFocus);
    m_header_widget->setAccessibleName(title);
    m_header_widget->setToolTip(QStringLiteral("Toggle section"));
    m_header_widget->setFixedHeight(24);
    m_header_widget->installEventFilter(this);
    auto *headerLayout = new QHBoxLayout(m_header_widget);
    headerLayout->setContentsMargins(8, 0, 8, 0);
    headerLayout->setSpacing(7);

    m_chevron_label = new MaterialIconLabel(QStringLiteral("expand_more"), m_header_widget);
    m_chevron_label->setProperty("iconRole", QStringLiteral("button"));
    m_chevron_label->setIconPixelSize(16);
    headerLayout->addWidget(m_chevron_label);

    m_title_label = new QLabel(title, m_header_widget);
    m_title_label->setObjectName(QStringLiteral("inspectorSectionTitle"));
    headerLayout->addWidget(m_title_label);
    headerLayout->addStretch();
    layout->addWidget(m_header_widget);

    m_content_widget = new QWidget(this);
    m_content_widget->setObjectName(QStringLiteral("inspectorSectionContent"));
    m_content_widget->setAttribute(Qt::WA_StyledBackground, true);
    m_content_layout = new QVBoxLayout(m_content_widget);
    m_content_layout->setContentsMargins(2, 3, 2, 5);
    m_content_layout->setSpacing(4);
    layout->addWidget(m_content_widget);

    refreshCollapsedState();
}

QVBoxLayout *InspectorSection::contentLayout() const
{
    return m_content_layout;
}

bool InspectorSection::isCollapsed() const
{
    return m_collapsed;
}

void InspectorSection::setTitle(const QString &title)
{
    m_title_label->setText(title);
    if (m_header_widget != nullptr)
    {
        m_header_widget->setAccessibleName(title);
    }
}

void InspectorSection::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed)
    {
        return;
    }

    m_collapsed = collapsed;
    refreshCollapsedState();
    emit collapsedChanged(m_collapsed);
}

bool InspectorSection::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_header_widget)
    {
        return QFrame::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonRelease)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton && m_header_widget->rect().contains(mouseEvent->pos()))
        {
            toggleCollapsed();
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton && m_header_widget->rect().contains(mouseEvent->pos()))
        {
            m_header_widget->setFocus(Qt::MouseFocusReason);
            return false;
        }
    }

    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter
            || keyEvent->key() == Qt::Key_Space)
        {
            toggleCollapsed();
            return true;
        }
    }

    return QFrame::eventFilter(watched, event);
}

void InspectorSection::toggleCollapsed()
{
    setCollapsed(!m_collapsed);
}

void InspectorSection::refreshCollapsedState()
{
    if (m_chevron_label != nullptr)
    {
        m_chevron_label->setIconName(m_collapsed ? QStringLiteral("chevron_right") : QStringLiteral("expand_more"));
    }

    if (m_content_widget != nullptr)
    {
        m_content_widget->setVisible(!m_collapsed);
    }

    setProperty("collapsed", m_collapsed);
    if (m_header_widget != nullptr)
    {
        m_header_widget->setProperty("collapsed", m_collapsed);
    }

    // Dynamic QSS properties do not repaint automatically after state flips.
    for (QWidget *widget : {static_cast<QWidget *>(this), m_header_widget, m_content_widget})
    {
        if (widget == nullptr)
        {
            continue;
        }

        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }
}

} // namespace PolyShow
