#include "ui/PillButton.h"

#include "ui/MaterialIconLabel.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QSize>
#include <QStyle>

namespace PolyShow
{

PillButton::PillButton(const QString &text, QWidget *parent)
    : QPushButton(parent)
{
    setObjectName(QStringLiteral("toolButton"));
    setCursor(Qt::PointingHandCursor);

    m_content_layout = new QHBoxLayout(this);
    m_content_layout->setContentsMargins(5, 0, 5, 0);
    m_content_layout->setSpacing(4);
    m_content_layout->setAlignment(Qt::AlignCenter);

    m_icon_label = new MaterialIconLabel(QString(), this);
    m_icon_label->setProperty("role", QStringLiteral("materialIcon"));
    m_icon_label->setProperty("iconRole", QStringLiteral("button"));
    m_icon_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_icon_label->setIconPixelSize(18);
    m_icon_label->setVisible(false);
    m_content_layout->addWidget(m_icon_label, 0, Qt::AlignCenter);

    m_text_label = new QLabel(this);
    m_text_label->setObjectName(QStringLiteral("buttonText"));
    m_text_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_content_layout->addWidget(m_text_label, 0, Qt::AlignCenter);

    setText(text);
    setVariant(Variant::Neutral);
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

void PillButton::setVariant(Variant variant)
{
    setProperty("buttonVariant", variantName(variant));
    style()->unpolish(this);
    style()->polish(this);
    refreshContentStyle();
}

void PillButton::setIconName(const QString &iconName)
{
    setProperty("materialIcon", iconName);
    m_icon_label->setIconName(iconName);
    m_icon_label->setVisible(!iconName.isEmpty());
    refreshContentLayout();
}

void PillButton::setText(const QString &text)
{
    m_display_text = text;
    QPushButton::setText(QString());
    refreshContentLayout();
}

QSize PillButton::sizeHint() const
{
    const int iconWidth = m_icon_label != nullptr && !m_icon_label->text().isEmpty() ? 18 : 0;
    if (iconWidth > 0 && m_display_text.isEmpty())
    {
        return QSize(30, 30);
    }

    const int spacing = iconWidth > 0 && !m_display_text.isEmpty() ? 4 : 0;
    const int textWidth = m_text_label == nullptr ? 0 : m_text_label->sizeHint().width();
    return QSize(qMax(30, 10 + iconWidth + spacing + textWidth), 30);
}

QSize PillButton::minimumSizeHint() const
{
    return sizeHint();
}

void PillButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::StyleChange)
    {
        refreshContentStyle();
    }
}

void PillButton::enterEvent(QEnterEvent *event)
{
    QPushButton::enterEvent(event);
    refreshContentStyle();
}

void PillButton::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);
    refreshContentStyle();
}

QString PillButton::variantName(Variant variant)
{
    switch (variant)
    {
    case Variant::Neutral:
        return QStringLiteral("neutral");
    case Variant::Primary:
        return QStringLiteral("primary");
    case Variant::Success:
        return QStringLiteral("success");
    default:
        return QStringLiteral("neutral");
    }
}

void PillButton::refreshContentLayout()
{
    if (m_content_layout != nullptr)
    {
        m_content_layout->setContentsMargins(m_display_text.isEmpty() ? 0 : 5, 0, m_display_text.isEmpty() ? 0 : 5, 0);
    }

    if (m_icon_label != nullptr)
    {
        m_icon_label->setIconPixelSize(18);
    }

    if (m_text_label != nullptr)
    {
        m_text_label->setText(m_display_text);
        m_text_label->setVisible(!m_display_text.isEmpty());
    }

    updateGeometry();
}

void PillButton::refreshContentStyle()
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
        widget->setProperty("buttonVariant", property("buttonVariant"));
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
    }
}

} // namespace PolyShow
