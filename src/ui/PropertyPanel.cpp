#include "ui/PropertyPanel.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

namespace PolyShow
{

namespace
{

/// Converts the internal render mode enum to UI text.
QString renderModeText(GeometryScene::RenderMode renderMode)
{
    switch (renderMode)
    {
    case GeometryScene::RenderMode::Solid:
        return QStringLiteral("Solid");
    case GeometryScene::RenderMode::Wireframe:
        return QStringLiteral("Wireframe");
    case GeometryScene::RenderMode::Points:
        return QStringLiteral("Points");
    default:
        return QStringLiteral("Unknown");
    }
}

} // namespace

/// Creates the property panel and its static controls.
PropertyPanel::PropertyPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel(QStringLiteral("Properties"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    layout->addWidget(titleLabel);

    auto *statsLayout = new QFormLayout();
    statsLayout->setLabelAlignment(Qt::AlignLeft);
    statsLayout->setFormAlignment(Qt::AlignTop);
    statsLayout->setHorizontalSpacing(10);
    statsLayout->setVerticalSpacing(8);

    // Use dedicated labels for every statistic so later updates stay simple.
    m_file_value_label = new QLabel(QStringLiteral("None"), this);
    m_file_value_label->setWordWrap(true);
    m_points_value_label = new QLabel(QStringLiteral("0"), this);
    m_polylines_value_label = new QLabel(QStringLiteral("0"), this);
    m_polygons_value_label = new QLabel(QStringLiteral("0"), this);
    m_render_mode_value_label = new QLabel(renderModeText(GeometryScene::RenderMode::Solid), this);

    statsLayout->addRow(QStringLiteral("File"), m_file_value_label);
    statsLayout->addRow(QStringLiteral("Points"), m_points_value_label);
    statsLayout->addRow(QStringLiteral("Polylines"), m_polylines_value_label);
    statsLayout->addRow(QStringLiteral("Polygons"), m_polygons_value_label);
    statsLayout->addRow(QStringLiteral("Render"), m_render_mode_value_label);
    layout->addLayout(statsLayout);

    // Separate the read-only stats from interactive display options.
    auto *divider = new QFrame(this);
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Sunken);
    layout->addWidget(divider);

    // Route grid visibility changes back to the scene through a signal.
    m_grid_check_box = new QCheckBox(QStringLiteral("Show grid"), this);
    m_grid_check_box->setChecked(true);
    connect(m_grid_check_box, &QCheckBox::toggled, this, &PropertyPanel::gridVisibilityChanged);
    layout->addWidget(m_grid_check_box);

    layout->addStretch(1);
}

/// Updates the file path shown at the top of the panel.
void PropertyPanel::setCurrentFile(const QString &filePath)
{
    m_file_value_label->setText(filePath.isEmpty() ? QStringLiteral("None") : filePath);
}

/// Updates the displayed geometry counters.
void PropertyPanel::setGeometryStats(int points, int polylines, int polygons)
{
    m_points_value_label->setText(QString::number(points));
    m_polylines_value_label->setText(QString::number(polylines));
    m_polygons_value_label->setText(QString::number(polygons));
}

/// Updates the current render mode label.
void PropertyPanel::setRenderMode(GeometryScene::RenderMode renderMode)
{
    m_render_mode_value_label->setText(renderModeText(renderMode));
}

} // namespace PolyShow
