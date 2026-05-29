#include "ui/LayerSidebar.h"

#include "ui/EditorPanelHeader.h"
#include "ui/IconButton.h"
#include "ui/MaterialIconLabel.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

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
constexpr int kVisibilityColumn = 0;
constexpr int kContentColumn = 1;

Qt::CheckState layerCheckState(const LayerData &layer)
{
    if (layer.primitives.isEmpty())
    {
        return Qt::Checked;
    }

    int visibleCount = 0;
    for (const LayerPrimitiveData &primitive : layer.primitives)
    {
        if (primitive.visible)
        {
            ++visibleCount;
        }
    }

    if (visibleCount == 0)
    {
        return Qt::Unchecked;
    }

    if (visibleCount == layer.primitives.size())
    {
        return Qt::Checked;
    }

    return Qt::PartiallyChecked;
}

Qt::CheckState layerCheckState(const QTreeWidgetItem *layerItem)
{
    if (layerItem == nullptr || layerItem->childCount() == 0)
    {
        return Qt::Checked;
    }

    int checkedCount = 0;
    for (int childIndex = 0; childIndex < layerItem->childCount(); ++childIndex)
    {
        if (layerItem->child(childIndex)->checkState(kVisibilityColumn) == Qt::Checked)
        {
            ++checkedCount;
        }
    }

    if (checkedCount == 0)
    {
        return Qt::Unchecked;
    }

    if (checkedCount == layerItem->childCount())
    {
        return Qt::Checked;
    }

    return Qt::PartiallyChecked;
}

void setLayerChildCheckStates(QTreeWidgetItem *layerItem, Qt::CheckState checkState)
{
    if (layerItem == nullptr)
    {
        return;
    }

    for (int childIndex = 0; childIndex < layerItem->childCount(); ++childIndex)
    {
        layerItem->child(childIndex)->setCheckState(kVisibilityColumn, checkState);
    }
}

QString layerText(const LayerData &layer)
{
    QString typeSuffix;
    switch (layer.layer_type)
    {
    case LayerType::ExternalFileNormal:
        typeSuffix = QStringLiteral("(File)");
        break;
    case LayerType::InternalNormal:
        typeSuffix = QStringLiteral("(Internal)");
        break;
    case LayerType::InternalIpc:
        typeSuffix = QStringLiteral("(IPC)");
        break;
    default:
        typeSuffix = QStringLiteral("(Layer)");
        break;
    }

    return QStringLiteral("%1 %2   %3")
        .arg(layer.display_name.isEmpty() ? QStringLiteral("Unnamed layer") : layer.display_name)
        .arg(typeSuffix)
        .arg(layer.primitives.size());
}

QString layerToolTip(const LayerData &layer)
{
    QString sourceText;
    switch (layer.layer_type)
    {
    case LayerType::ExternalFileNormal:
        sourceText = QStringLiteral("File layer");
        break;
    case LayerType::InternalNormal:
        sourceText = QStringLiteral("Internal layer");
        break;
    case LayerType::InternalIpc:
        sourceText = QStringLiteral("IPC layer");
        break;
    default:
        sourceText = QStringLiteral("Layer");
        break;
    }

    if (layer.file_path.isEmpty())
    {
        return sourceText;
    }

    return QStringLiteral("%1\n%2").arg(sourceText, layer.file_path);
}

QString layerIconName(const LayerData &layer)
{
    switch (layer.layer_type)
    {
    case LayerType::InternalIpc:
        return QStringLiteral("settings_input_component");
    case LayerType::ExternalFileNormal:
    case LayerType::InternalNormal:
    default:
        return QStringLiteral("folder");
    }
}

QString primitiveIconName(const LayerData &layer, int primitiveIndex)
{
    const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
    switch (primitive.reference.kind)
    {
    case PrimitiveKind::Point:
        return QStringLiteral("radio_button_checked");
    case PrimitiveKind::Polyline:
        if (primitive.reference.index >= 0 && primitive.reference.index < layer.geometry.polylines.size()
            && layer.geometry.polylines.at(primitive.reference.index).vertices.size() == 2)
        {
            return QStringLiteral("show_chart");
        }
        return QStringLiteral("timeline");
    case PrimitiveKind::Polygon:
        return QStringLiteral("pentagon");
    default:
        return QStringLiteral("deployed_code");
    }
}

QString footerSummary(const DocumentData &documentData)
{
    int visiblePrimitiveCount = 0;
    for (const LayerData &layer : documentData.layers)
    {
        for (const LayerPrimitiveData &primitive : layer.primitives)
        {
            if (primitive.visible)
            {
                ++visiblePrimitiveCount;
            }
        }
    }

    return QStringLiteral("%1 layers  /  %2 visible primitives")
        .arg(documentData.layers.size())
        .arg(visiblePrimitiveCount);
}

void refreshStyledWidget(QWidget *widget)
{
    if (widget == nullptr)
    {
        return;
    }

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

QWidget *createTreeRowWidget(const QString &iconName, const QString &text, bool isLayer, QWidget *parent)
{
    auto *row = new QWidget(parent);
    row->setObjectName(QStringLiteral("outlinerRow"));
    row->setProperty("rowKind", isLayer ? QStringLiteral("layer") : QStringLiteral("primitive"));
    row->setProperty("rowState", QStringLiteral("normal"));
    row->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 6, 0);
    layout->setSpacing(6);

    auto *icon = new MaterialIconLabel(iconName, row);
    icon->setProperty("iconRole", QStringLiteral("outliner"));
    icon->setProperty("rowState", QStringLiteral("normal"));
    icon->setAttribute(Qt::WA_TransparentForMouseEvents);
    icon->setIconPixelSize(18);
    layout->addWidget(icon);

    auto *label = new QLabel(text, row);
    label->setObjectName(QStringLiteral("outlinerRowText"));
    label->setProperty("rowKind", isLayer ? QStringLiteral("layer") : QStringLiteral("primitive"));
    label->setProperty("rowState", QStringLiteral("normal"));
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setTextFormat(Qt::PlainText);
    layout->addWidget(label, 1);

    return row;
}

void refreshTreeRowWidget(QWidget *row, bool selected)
{
    if (row == nullptr)
    {
        return;
    }

    const QString rowState = selected ? QStringLiteral("selected") : QStringLiteral("normal");
    row->setProperty("rowState", rowState);
    refreshStyledWidget(row);

    const QList<QWidget *> children = row->findChildren<QWidget *>();
    for (QWidget *child : children)
    {
        child->setProperty("rowState", rowState);
        refreshStyledWidget(child);
    }
}

} // namespace

LayerSidebar::LayerSidebar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("outliner"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new EditorPanelHeader(QStringLiteral("account_tree"), QStringLiteral("Outliner"), this);

    m_new_layer_button = new IconButton(QStringLiteral("add"), QString(), header);
    m_new_layer_button->setObjectName(QStringLiteral("iconButton"));
    m_new_layer_button->setCursor(Qt::PointingHandCursor);
    m_new_layer_button->setToolTip(QStringLiteral("Create Layer"));
    header->actionsLayout()->addWidget(m_new_layer_button);

    m_export_layer_button = new IconButton(QStringLiteral("ios_share"), QString(), header);
    m_export_layer_button->setObjectName(QStringLiteral("iconButton"));
    m_export_layer_button->setCursor(Qt::PointingHandCursor);
    m_export_layer_button->setToolTip(QStringLiteral("Export Active Layer"));
    header->actionsLayout()->addWidget(m_export_layer_button);

    m_search_button = new IconButton(QStringLiteral("search"), QString(), header);
    m_search_button->setObjectName(QStringLiteral("iconButton"));
    m_search_button->setCheckable(true);
    m_search_button->setCursor(Qt::PointingHandCursor);
    m_search_button->setToolTip(QStringLiteral("Search"));
    header->actionsLayout()->addWidget(m_search_button);
    layout->addWidget(header);

    m_search_line_edit = new QLineEdit(this);
    m_search_line_edit->setObjectName(QStringLiteral("outlinerSearchInput"));
    m_search_line_edit->setPlaceholderText(QStringLiteral("Filter layers and primitives"));
    m_search_line_edit->setVisible(false);
    layout->addWidget(m_search_line_edit);

    m_section_label = new QLabel(QStringLiteral("Layers"), this);
    m_section_label->setProperty("role", QStringLiteral("sectionTitle"));
    m_section_label->setObjectName(QStringLiteral("outlinerSectionLabel"));
    m_section_label->setVisible(false);
    layout->addWidget(m_section_label);

    m_tree_widget = new QTreeWidget(this);
    m_tree_widget->setObjectName(QStringLiteral("outlinerTree"));
    m_tree_widget->setColumnCount(2);
    m_tree_widget->setHeaderHidden(true);
    m_tree_widget->setRootIsDecorated(true);
    m_tree_widget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree_widget->setIndentation(18);
    m_tree_widget->setAlternatingRowColors(false);
    m_tree_widget->header()->setSectionResizeMode(kVisibilityColumn, QHeaderView::Fixed);
    m_tree_widget->header()->resizeSection(kVisibilityColumn, 42);
    m_tree_widget->header()->setSectionResizeMode(kContentColumn, QHeaderView::Stretch);
    layout->addWidget(m_tree_widget, 1);

    m_footer_label = new QLabel(this);
    m_footer_label->setObjectName(QStringLiteral("outlinerSummary"));
    m_footer_label->setProperty("role", QStringLiteral("mono"));
    m_footer_label->setFixedHeight(26);
    layout->addWidget(m_footer_label);

    connect(m_search_button, &QPushButton::toggled, this, [this](bool checked) {
        setSearchExpanded(checked);
    });
    connect(m_new_layer_button, &QPushButton::clicked, this, &LayerSidebar::createLayerRequested);
    connect(m_export_layer_button, &QPushButton::clicked, this, &LayerSidebar::exportLayerRequested);
    connect(m_search_line_edit, &QLineEdit::textChanged, this, [this]() {
        applyFilter();
    });
    connect(m_tree_widget, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int column) {
        if (m_is_rebuilding_tree || m_is_syncing_visibility || m_is_syncing_selection || column != kVisibilityColumn
            || item == nullptr)
        {
            return;
        }

        const int itemKind = item->data(kVisibilityColumn, kItemKindRole).toInt();
        const int layerIndex = item->data(kVisibilityColumn, kLayerIndexRole).toInt();

        if (itemKind == ItemKindLayer)
        {
            const Qt::CheckState nextCheckState =
                item->checkState(kVisibilityColumn) == Qt::Unchecked ? Qt::Unchecked : Qt::Checked;

            m_is_syncing_visibility = true;
            item->setCheckState(kVisibilityColumn, nextCheckState);
            setLayerChildCheckStates(item, nextCheckState);
            m_is_syncing_visibility = false;

            emit layerVisibilityChanged(layerIndex, nextCheckState == Qt::Checked);
            return;
        }

        if (itemKind == ItemKindPrimitive)
        {
            const bool visible = item->checkState(kVisibilityColumn) == Qt::Checked;
            const int primitiveIndex = item->data(kVisibilityColumn, kPrimitiveIndexRole).toInt();

            m_is_syncing_visibility = true;
            QTreeWidgetItem *layerItem = item->parent();
            if (layerItem != nullptr)
            {
                layerItem->setCheckState(kVisibilityColumn, layerCheckState(layerItem));
            }
            m_is_syncing_visibility = false;

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
            refreshTreeRowStyles();
            emit selectionChanged(m_selection_state);
            return;
        }

        SelectionState nextSelection;
        const int itemKind = current->data(kVisibilityColumn, kItemKindRole).toInt();
        nextSelection.layer_index = current->data(kVisibilityColumn, kLayerIndexRole).toInt();
        nextSelection.primitive_index = current->data(kVisibilityColumn, kPrimitiveIndexRole).toInt();
        nextSelection.kind = itemKind == ItemKindPrimitive ? SelectionKind::Primitive : SelectionKind::Layer;

        if (nextSelection != m_selection_state)
        {
            m_selection_state = nextSelection;
            refreshTreeRowStyles();
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

    syncTreeCheckStates();
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
        refreshTreeRowStyles();
        m_is_syncing_selection = false;
        return;
    }

    m_tree_widget->setCurrentItem(item);
    item->setSelected(true);
    refreshTreeRowStyles();
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
        layerItem->setText(kContentColumn, layerText(layer));
        layerItem->setToolTip(kVisibilityColumn, layerToolTip(layer));
        layerItem->setToolTip(kContentColumn, layerToolTip(layer));
        layerItem->setFlags(layerItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        layerItem->setData(kVisibilityColumn, kItemKindRole, ItemKindLayer);
        layerItem->setData(kVisibilityColumn, kLayerIndexRole, layerIndex);
        layerItem->setData(kVisibilityColumn, kPrimitiveIndexRole, -1);

        for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
        {
            const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);

            auto *primitiveItem = new QTreeWidgetItem(layerItem);
            primitiveItem->setText(kContentColumn, primitive.display_name);
            primitiveItem->setFlags(
                primitiveItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            primitiveItem->setCheckState(kVisibilityColumn, primitive.visible ? Qt::Checked : Qt::Unchecked);
            primitiveItem->setData(kVisibilityColumn, kItemKindRole, ItemKindPrimitive);
            primitiveItem->setData(kVisibilityColumn, kLayerIndexRole, layerIndex);
            primitiveItem->setData(kVisibilityColumn, kPrimitiveIndexRole, primitiveIndex);
        }

        layerItem->setCheckState(kVisibilityColumn, layerCheckState(layer));
        layerItem->setExpanded(true);
        m_tree_widget->setItemWidget(
            layerItem,
            kContentColumn,
            createTreeRowWidget(layerIconName(layer), layerText(layer), true, m_tree_widget));

        for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
        {
            QTreeWidgetItem *primitiveItem = layerItem->child(primitiveIndex);
            if (primitiveItem == nullptr)
            {
                continue;
            }

            const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
            m_tree_widget->setItemWidget(
                primitiveItem,
                kContentColumn,
                createTreeRowWidget(
                    primitiveIconName(layer, primitiveIndex), primitive.display_name, false, m_tree_widget));
        }
    }

    refreshTreeRowStyles();
    m_is_rebuilding_tree = false;
}

void LayerSidebar::syncTreeCheckStates()
{
    m_is_syncing_visibility = true;
    const QSignalBlocker blocker(m_tree_widget);

    const int layerCount = std::min(m_tree_widget->topLevelItemCount(), static_cast<int>(m_document_data.layers.size()));
    for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        const LayerData &layer = m_document_data.layers.at(layerIndex);
        QTreeWidgetItem *layerItem = m_tree_widget->topLevelItem(layerIndex);
        if (layerItem == nullptr)
        {
            continue;
        }

        const int primitiveCount = std::min(layerItem->childCount(), static_cast<int>(layer.primitives.size()));
        for (int primitiveIndex = 0; primitiveIndex < primitiveCount; ++primitiveIndex)
        {
            QTreeWidgetItem *primitiveItem = layerItem->child(primitiveIndex);
            if (primitiveItem == nullptr)
            {
                continue;
            }

            primitiveItem->setCheckState(
                kVisibilityColumn,
                layer.primitives.at(primitiveIndex).visible ? Qt::Checked : Qt::Unchecked);
        }

        layerItem->setCheckState(kVisibilityColumn, layerCheckState(layer));
    }

    m_is_syncing_visibility = false;
}

void LayerSidebar::applyFilter()
{
    const QString query = m_search_line_edit->text().trimmed().toLower();
    const bool hasQuery = !query.isEmpty();

    m_section_label->setText(hasQuery ? QStringLiteral("Search Results") : QStringLiteral("Layers"));

    for (int layerIndex = 0; layerIndex < m_tree_widget->topLevelItemCount(); ++layerIndex)
    {
        QTreeWidgetItem *layerItem = m_tree_widget->topLevelItem(layerIndex);
        if (layerItem == nullptr)
        {
            continue;
        }

        const bool layerMatches = layerItem->text(kContentColumn).toLower().contains(query);
        bool anyVisibleChild = false;

        for (int childIndex = 0; childIndex < layerItem->childCount(); ++childIndex)
        {
            QTreeWidgetItem *childItem = layerItem->child(childIndex);
            const bool childMatches =
                !hasQuery || layerMatches || childItem->text(kContentColumn).toLower().contains(query);
            childItem->setHidden(!childMatches);
            anyVisibleChild = anyVisibleChild || childMatches;
        }

        const bool showLayer = !hasQuery || layerMatches || anyVisibleChild;
        layerItem->setHidden(!showLayer);
        layerItem->setExpanded(hasQuery || layerItem->isExpanded());
    }

    updateFooter();
    refreshTreeRowStyles();
}

void LayerSidebar::refreshTreeRowStyles()
{
    for (int layerIndex = 0; layerIndex < m_tree_widget->topLevelItemCount(); ++layerIndex)
    {
        QTreeWidgetItem *layerItem = m_tree_widget->topLevelItem(layerIndex);
        if (layerItem == nullptr)
        {
            continue;
        }

        refreshTreeRowWidget(m_tree_widget->itemWidget(layerItem, kContentColumn), layerItem->isSelected());

        for (int childIndex = 0; childIndex < layerItem->childCount(); ++childIndex)
        {
            QTreeWidgetItem *childItem = layerItem->child(childIndex);
            if (childItem == nullptr)
            {
                continue;
            }

            refreshTreeRowWidget(m_tree_widget->itemWidget(childItem, kContentColumn), childItem->isSelected());
        }
    }
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

        const int itemLayerIndex = layerItem->data(kVisibilityColumn, kLayerIndexRole).toInt();
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
            if (childItem->data(kVisibilityColumn, kPrimitiveIndexRole).toInt() == selectionState.primitive_index)
            {
                return childItem;
            }
        }
    }

    return nullptr;
}

} // namespace PolyShow
