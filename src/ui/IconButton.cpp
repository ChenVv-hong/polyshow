#include "ui/IconButton.h"

#include "ui/MaterialIconLabel.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QSize>
#include <QStyle>

namespace PolyShow
{

IconButton::IconButton(const QString &iconName, const QString &text, QWidget *parent)
    : QPushButton(parent)
{
    setObjectName(text.isEmpty() ? QStringLiteral("iconButton") : QStringLiteral("toolButton"));
    setCursor(Qt::PointingHandCursor);

    m_content_layout = new QHBoxLayout(this);
    m_content_layout->setContentsMargins(text.isEmpty() ? 0 : 8, 0, text.isEmpty() ? 0 : 8, 0);
    m_content_layout->setSpacing(5);
    m_content_layout->setAlignment(Qt::AlignCenter);

    m_icon_label = new MaterialIconLabel(iconName, this);
    m_icon_label->setProperty("role", QStringLiteral("materialIcon"));
    m_icon_label->setProperty("iconRole", QStringLiteral("button"));
    m_icon_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_icon_label->setIconPixelSize(text.isEmpty() ? 18 : 20);
    m_content_layout->addWidget(m_icon_label, 0, Qt::AlignCenter);

    m_text_label = new QLabel(this);
    m_text_label->setObjectName(QStringLiteral("buttonText"));
    m_text_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_content_layout->addWidget(m_text_label, 0, Qt::AlignCenter);

    setText(text);
    setIconName(iconName);
    refreshContentStyle();

    connect(this, &QPushButton::toggled, this, [this]() {
        refreshContentStyle();
    });
    connect(this, &QPushButton::pressed, this, [this]() {
        refreshContentStyle();
    });
    connect(this, &QPushButton::released, this, [this]() {
        refreshContentStyle();
    });
}

void IconButton::setIconName(const QString &iconName)
{
    setProperty("materialIcon", iconName);
    m_icon_label->setIconName(iconName);
}

void IconButton::setText(const QString &text)
{
    m_display_text = text;
    QPushButton::setText(QString());
    refreshContentLayout();
}

QSize IconButton::sizeHint() const
{
    if (m_display_text.isEmpty())
    {
        return QSize(30, 30);
    }

    const int textWidth = m_text_label == nullptr ? 0 : m_text_label->sizeHint().width();
    return QSize(qMax(30, 8 + 20 + 5 + textWidth + 8), 30);
}

QSize IconButton::minimumSizeHint() const
{
    return sizeHint();
}

void IconButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::StyleChange)
    {
        refreshContentStyle();
    }
}

void IconButton::enterEvent(QEnterEvent *event)
{
    QPushButton::enterEvent(event);
    refreshContentStyle();
}

void IconButton::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);
    refreshContentStyle();
}

void IconButton::refreshContentLayout()
{
    if (m_content_layout != nullptr)
    {
        m_content_layout->setContentsMargins(m_display_text.isEmpty() ? 0 : 8, 0, m_display_text.isEmpty() ? 0 : 8, 0);
    }

    if (m_icon_label != nullptr)
    {
        m_icon_label->setIconPixelSize(m_display_text.isEmpty() ? 18 : 20);
    }

    if (m_text_label != nullptr)
    {
        m_text_label->setText(m_display_text);
        m_text_label->setVisible(!m_display_text.isEmpty());
    }

    updateGeometry();
}

void IconButton::refreshContentStyle()
{
    const QString state = !isEnabled()
        ? QStringLiteral("disabled")
        : ((isChecked() || underMouse()) ? QStringLiteral("active") : QStringLiteral("normal"));

    for (QWidget *widget : {static_cast<QWidget *>(m_icon_label), static_cast<QWidget *>(m_text_label)})
    {
        if (widget == nullptr)
        {
            continue;
        }

        widget->setProperty("buttonState", state);
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }
}

} // namespace PolyShow
