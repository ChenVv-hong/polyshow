#include "ui/PropertyPanel.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace PolyShow
{

namespace
{

enum ItemKind
{
    ItemKindLayer = 1,
    ItemKindPrimitive = 2
};

constexpr int kItemKindRole = Qt::UserRole + 1;
constexpr int kLayerIndexRole = Qt::UserRole + 2;
constexpr int kPrimitiveIndexRole = Qt::UserRole + 3;

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

/// Returns the visible label for one layer entry.
QString layerDisplayText(const LayerData &layer, int layerIndex)
{
    if (!layer.display_name.isEmpty())
    {
        return layer.display_name;
    }

    return QStringLiteral("Layer %1").arg(layerIndex + 1);
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

    statsLayout->addRow(QStringLiteral("Files"), m_file_value_label);
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

    auto *layersLabel = new QLabel(QStringLiteral("Layers"), this);
    QFont layersFont = layersLabel->font();
    layersFont.setBold(true);
    layersLabel->setFont(layersFont);
    layout->addWidget(layersLabel);

    m_layer_tree_widget = new QTreeWidget(this);
    m_layer_tree_widget->setColumnCount(1);
    m_layer_tree_widget->setHeaderHidden(true);
    m_layer_tree_widget->setRootIsDecorated(true);
    m_layer_tree_widget->setAlternatingRowColors(true);
    m_layer_tree_widget->header()->setStretchLastSection(true);
    connect(
        m_layer_tree_widget,
        &QTreeWidget::itemChanged,
        this,
        [this](QTreeWidgetItem *item, int column) {
            if (column != 0 || item == nullptr)
            {
                return;
            }

            const int itemKind = item->data(0, kItemKindRole).toInt();
            const int layerIndex = item->data(0, kLayerIndexRole).toInt();
            const bool visible = item->checkState(0) == Qt::Checked;

            if (itemKind == ItemKindLayer)
            {
                emit layerVisibilityChanged(layerIndex, visible);
                return;
            }

            if (itemKind == ItemKindPrimitive)
            {
                const int primitiveIndex = item->data(0, kPrimitiveIndexRole).toInt();
                emit primitiveVisibilityChanged(layerIndex, primitiveIndex, visible);
            }
        });
    layout->addWidget(m_layer_tree_widget, 1);
}

/// Rebuilds the document summary widgets from the latest imported document.
void PropertyPanel::setDocumentData(const DocumentData &documentData)
{
    updateLoadedFilesSummary(documentData);
    rebuildLayerTree(documentData);
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

/// Updates the compact loaded-file summary at the top of the panel.
void PropertyPanel::updateLoadedFilesSummary(const DocumentData &documentData)
{
    QStringList displayNames;
    QStringList toolTips;

    for (const LayerData &layer : documentData.layers)
    {
        displayNames.append(layer.display_name.isEmpty() ? QStringLiteral("Unnamed layer") : layer.display_name);
        toolTips.append(layer.file_path);
    }

    const QString summaryText = displayNames.isEmpty() ? QStringLiteral("None") : displayNames.join(QStringLiteral("\n"));
    m_file_value_label->setText(summaryText);
    m_file_value_label->setToolTip(toolTips.join(QStringLiteral("\n")));
}

/// Rebuilds the layer tree so it mirrors the latest document state.
void PropertyPanel::rebuildLayerTree(const DocumentData &documentData)
{
    const QSignalBlocker blocker(m_layer_tree_widget);
    m_layer_tree_widget->clear();

    for (int layerIndex = 0; layerIndex < documentData.layers.size(); ++layerIndex)
    {
        const LayerData &layer = documentData.layers.at(layerIndex);

        auto *layerItem = new QTreeWidgetItem(m_layer_tree_widget);
        layerItem->setText(0, layerDisplayText(layer, layerIndex));
        layerItem->setToolTip(0, layer.file_path);
        layerItem->setFlags(layerItem->flags() | Qt::ItemIsUserCheckable);
        layerItem->setCheckState(0, layer.visible ? Qt::Checked : Qt::Unchecked);
        layerItem->setData(0, kItemKindRole, ItemKindLayer);
        layerItem->setData(0, kLayerIndexRole, layerIndex);

        for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
        {
            const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);

            auto *primitiveItem = new QTreeWidgetItem(layerItem);
            primitiveItem->setText(0, primitive.display_name);
            primitiveItem->setFlags(primitiveItem->flags() | Qt::ItemIsUserCheckable);
            primitiveItem->setCheckState(0, primitive.visible ? Qt::Checked : Qt::Unchecked);
            primitiveItem->setData(0, kItemKindRole, ItemKindPrimitive);
            primitiveItem->setData(0, kLayerIndexRole, layerIndex);
            primitiveItem->setData(0, kPrimitiveIndexRole, primitiveIndex);
        }

        layerItem->setExpanded(true);
    }
}

} // namespace PolyShow
