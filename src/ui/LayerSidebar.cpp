#include "ui/LayerSidebar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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

QString layerText(const LayerData &layer)
{
    return QStringLiteral("%1   %2")
        .arg(layer.display_name.isEmpty() ? QStringLiteral("Unnamed layer") : layer.display_name)
        .arg(layer.primitives.size());
}

QString footerSummary(const DocumentData &documentData)
{
    int visiblePrimitiveCount = 0;
    for (const LayerData &layer : documentData.layers)
    {
        if (!layer.visible)
        {
            continue;
        }

        for (const LayerPrimitiveData &primitive : layer.primitives)
        {
            if (primitive.visible)
            {
                ++visiblePrimitiveCount;
            }
        }
    }

    return QStringLiteral("%1 files  /  %2 visible primitives")
        .arg(documentData.layers.size())
        .arg(visiblePrimitiveCount);
}

} // namespace

LayerSidebar::LayerSidebar(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    auto *titleLabel = new QLabel(QStringLiteral("Layers"), this);
    titleLabel->setProperty("role", QStringLiteral("panelTitle"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_search_button = new QPushButton(QStringLiteral("S"), this);
    m_search_button->setCheckable(true);
    m_search_button->setCursor(Qt::PointingHandCursor);
    m_search_button->setFixedSize(28, 28);
    m_search_button->setToolTip(QStringLiteral("Search"));
    headerLayout->addWidget(m_search_button);
    layout->addLayout(headerLayout);

    m_search_line_edit = new QLineEdit(this);
    m_search_line_edit->setPlaceholderText(QStringLiteral("Filter files and primitives"));
    m_search_line_edit->setVisible(false);
    layout->addWidget(m_search_line_edit);

    m_section_label = new QLabel(QStringLiteral("Imported Files"), this);
    m_section_label->setProperty("role", QStringLiteral("sectionTitle"));
    layout->addWidget(m_section_label);

    m_tree_widget = new QTreeWidget(this);
    m_tree_widget->setColumnCount(1);
    m_tree_widget->setHeaderHidden(true);
    m_tree_widget->setRootIsDecorated(true);
    m_tree_widget->setIndentation(18);
    m_tree_widget->setAlternatingRowColors(false);
    layout->addWidget(m_tree_widget, 1);

    m_footer_label = new QLabel(this);
    m_footer_label->setProperty("role", QStringLiteral("mono"));
    layout->addWidget(m_footer_label);

    connect(m_search_button, &QPushButton::toggled, this, [this](bool checked) {
        setSearchExpanded(checked);
    });
    connect(m_search_line_edit, &QLineEdit::textChanged, this, [this]() {
        applyFilter();
    });
    connect(m_tree_widget, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int column) {
        if (m_is_rebuilding_tree || m_is_syncing_selection || column != 0 || item == nullptr)
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
    connect(m_tree_widget, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *current, QTreeWidgetItem *) {
        if (m_is_rebuilding_tree || m_is_syncing_selection)
        {
            return;
        }

        if (current == nullptr)
        {
            m_selection_state = SelectionState {};
            emit selectionChanged(m_selection_state);
            return;
        }

        SelectionState nextSelection;
        const int itemKind = current->data(0, kItemKindRole).toInt();
        nextSelection.layer_index = current->data(0, kLayerIndexRole).toInt();
        nextSelection.primitive_index = current->data(0, kPrimitiveIndexRole).toInt();
        nextSelection.kind = itemKind == ItemKindPrimitive ? SelectionKind::Primitive : SelectionKind::Layer;

        if (nextSelection != m_selection_state)
        {
            m_selection_state = nextSelection;
            emit selectionChanged(m_selection_state);
        }
    });

    updateFooter();
}

void LayerSidebar::setDocumentData(const DocumentData &documentData, bool rebuildTreeItems)
{
    m_document_data = documentData;
    if (rebuildTreeItems)
    {
        rebuildTree();
        applyFilter();
        updateFooter();
        setSelectionState(m_selection_state);
        return;
    }

    updateFooter();
}

void LayerSidebar::setSelectionState(const SelectionState &selectionState)
{
    m_is_syncing_selection = true;
    m_selection_state = selectionState;

    const QSignalBlocker blocker(m_tree_widget);
    QTreeWidgetItem *item = findItem(selectionState);
    if (item == nullptr)
    {
        m_tree_widget->clearSelection();
        m_tree_widget->setCurrentItem(nullptr);
        m_is_syncing_selection = false;
        return;
    }

    m_tree_widget->setCurrentItem(item);
    item->setSelected(true);
    m_is_syncing_selection = false;
}

void LayerSidebar::setSearchExpanded(bool expanded)
{
    m_is_search_expanded = expanded;
    m_search_line_edit->setVisible(expanded);

    if (!expanded)
    {
        const QSignalBlocker blocker(m_search_line_edit);
        m_search_line_edit->clear();
    }
    else
    {
        m_search_line_edit->setFocus(Qt::OtherFocusReason);
    }

    applyFilter();
}

void LayerSidebar::updateFooter()
{
    if (m_is_search_expanded && !m_search_line_edit->text().trimmed().isEmpty())
    {
        int matchCount = 0;
        for (int layerIndex = 0; layerIndex < m_tree_widget->topLevelItemCount(); ++layerIndex)
        {
            QTreeWidgetItem *layerItem = m_tree_widget->topLevelItem(layerIndex);
            if (layerItem == nullptr || layerItem->isHidden())
            {
                continue;
            }

            ++matchCount;
            for (int childIndex = 0; childIndex < layerItem->childCount(); ++childIndex)
            {
                if (!layerItem->child(childIndex)->isHidden())
                {
                    ++matchCount;
                }
            }
        }

        m_footer_label->setText(
            QStringLiteral("Query: %1  /  %2 matches").arg(m_search_line_edit->text(), QString::number(matchCount)));
        return;
    }

    m_footer_label->setText(footerSummary(m_document_data));
}

void LayerSidebar::rebuildTree()
{
    m_is_rebuilding_tree = true;
    const QSignalBlocker blocker(m_tree_widget);
    m_tree_widget->clear();

    for (int layerIndex = 0; layerIndex < m_document_data.layers.size(); ++layerIndex)
    {
        const LayerData &layer = m_document_data.layers.at(layerIndex);

        auto *layerItem = new QTreeWidgetItem(m_tree_widget);
        layerItem->setText(0, layerText(layer));
        layerItem->setToolTip(0, layer.file_path);
        layerItem->setFlags(layerItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        layerItem->setCheckState(0, layer.visible ? Qt::Checked : Qt::Unchecked);
        layerItem->setData(0, kItemKindRole, ItemKindLayer);
        layerItem->setData(0, kLayerIndexRole, layerIndex);
        layerItem->setData(0, kPrimitiveIndexRole, -1);

        for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
        {
            const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);

            auto *primitiveItem = new QTreeWidgetItem(layerItem);
            primitiveItem->setText(0, primitive.display_name);
            primitiveItem->setFlags(
                primitiveItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            primitiveItem->setCheckState(0, primitive.visible ? Qt::Checked : Qt::Unchecked);
            primitiveItem->setData(0, kItemKindRole, ItemKindPrimitive);
            primitiveItem->setData(0, kLayerIndexRole, layerIndex);
            primitiveItem->setData(0, kPrimitiveIndexRole, primitiveIndex);
        }

        layerItem->setExpanded(true);
    }

    m_is_rebuilding_tree = false;
}

void LayerSidebar::applyFilter()
{
    const QString query = m_search_line_edit->text().trimmed().toLower();
    const bool hasQuery = !query.isEmpty();

    m_section_label->setText(hasQuery ? QStringLiteral("Search Results") : QStringLiteral("Imported Files"));

    for (int layerIndex = 0; layerIndex < m_tree_widget->topLevelItemCount(); ++layerIndex)
    {
        QTreeWidgetItem *layerItem = m_tree_widget->topLevelItem(layerIndex);
        if (layerItem == nullptr)
        {
            continue;
        }

        const bool layerMatches = layerItem->text(0).toLower().contains(query);
        bool anyVisibleChild = false;

        for (int childIndex = 0; childIndex < layerItem->childCount(); ++childIndex)
        {
            QTreeWidgetItem *childItem = layerItem->child(childIndex);
            const bool childMatches = !hasQuery || layerMatches || childItem->text(0).toLower().contains(query);
            childItem->setHidden(!childMatches);
            anyVisibleChild = anyVisibleChild || childMatches;
        }

        const bool showLayer = !hasQuery || layerMatches || anyVisibleChild;
        layerItem->setHidden(!showLayer);
        layerItem->setExpanded(hasQuery || layerItem->isExpanded());
    }

    updateFooter();
}

QTreeWidgetItem *LayerSidebar::findItem(const SelectionState &selectionState) const
{
    if (selectionState.kind == SelectionKind::None || selectionState.layer_index < 0)
    {
        return nullptr;
    }

    for (int layerIndex = 0; layerIndex < m_tree_widget->topLevelItemCount(); ++layerIndex)
    {
        QTreeWidgetItem *layerItem = m_tree_widget->topLevelItem(layerIndex);
        if (layerItem == nullptr)
        {
            continue;
        }

        const int itemLayerIndex = layerItem->data(0, kLayerIndexRole).toInt();
        if (itemLayerIndex != selectionState.layer_index)
        {
            continue;
        }

        if (selectionState.kind == SelectionKind::Layer)
        {
            return layerItem;
        }

        for (int childIndex = 0; childIndex < layerItem->childCount(); ++childIndex)
        {
            QTreeWidgetItem *childItem = layerItem->child(childIndex);
            if (childItem->data(0, kPrimitiveIndexRole).toInt() == selectionState.primitive_index)
            {
                return childItem;
            }
        }
    }

    return nullptr;
}

} // namespace PolyShow
