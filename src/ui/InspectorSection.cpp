#include "ui/InspectorSection.h"

#include "ui/MaterialIconLabel.h"

#include <QHBoxLayout>
#include <QLabel>
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

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("inspectorSectionHeader"));
    header->setAttribute(Qt::WA_StyledBackground, true);
    header->setFixedHeight(24);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(8, 0, 8, 0);
    headerLayout->setSpacing(7);

    auto *chevron = new MaterialIconLabel(QStringLiteral("expand_more"), header);
    chevron->setIconPixelSize(16);
    headerLayout->addWidget(chevron);

    m_title_label = new QLabel(title, header);
    m_title_label->setObjectName(QStringLiteral("inspectorSectionTitle"));
    headerLayout->addWidget(m_title_label);
    headerLayout->addStretch();
    layout->addWidget(header);

    auto *content = new QWidget(this);
    content->setObjectName(QStringLiteral("inspectorSectionContent"));
    content->setAttribute(Qt::WA_StyledBackground, true);
    m_content_layout = new QVBoxLayout(content);
    m_content_layout->setContentsMargins(2, 3, 2, 5);
    m_content_layout->setSpacing(4);
    layout->addWidget(content);
}

QVBoxLayout *InspectorSection::contentLayout() const
{
    return m_content_layout;
}

void InspectorSection::setTitle(const QString &title)
{
    m_title_label->setText(title);
}

} // namespace PolyShow
