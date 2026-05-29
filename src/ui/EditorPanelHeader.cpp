#include "ui/EditorPanelHeader.h"

#include "ui/MaterialIconLabel.h"

#include <QHBoxLayout>
#include <QLabel>

namespace PolyShow
{

EditorPanelHeader::EditorPanelHeader(const QString &iconName, const QString &title, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("editorPanelHeader"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(32);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(8);

    m_icon_label = new MaterialIconLabel(iconName, this);
    layout->addWidget(m_icon_label);

    m_title_label = new QLabel(title, this);
    m_title_label->setObjectName(QStringLiteral("panelTitle"));
    layout->addWidget(m_title_label);
    layout->addStretch();

    m_actions_layout = new QHBoxLayout();
    m_actions_layout->setContentsMargins(0, 0, 0, 0);
    m_actions_layout->setSpacing(6);
    layout->addLayout(m_actions_layout);
}

QHBoxLayout *EditorPanelHeader::actionsLayout() const
{
    return m_actions_layout;
}

} // namespace PolyShow
