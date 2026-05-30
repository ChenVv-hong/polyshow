#include "ui/OutlinerTreeModel.h"

#include <QtGlobal>

#include <algorithm>

namespace PolyShow
{

OutlinerTreeModel::OutlinerTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root_node(std::make_unique<OutlinerNode>())
{
}

QModelIndex OutlinerTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column != 0)
    {
        return QModelIndex();
    }

    const OutlinerNode *parentNode = nodeFromIndex(parent);
    if (parentNode == nullptr || row >= static_cast<int>(parentNode->children.size()))
    {
        return QModelIndex();
    }

    return createIndex(row, column, parentNode->children.at(row).get());
}

QModelIndex OutlinerTreeModel::parent(const QModelIndex &child) const
{
    const OutlinerNode *node = nodeFromIndex(child);
    if (node == nullptr || node->parent == nullptr || node->parent == m_root_node.get())
    {
        return QModelIndex();
    }

    return indexForNode(node->parent);
}

int OutlinerTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
    {
        return 0;
    }

    const OutlinerNode *node = nodeFromIndex(parent);
    return node == nullptr ? 0 : static_cast<int>(node->children.size());
}

int OutlinerTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant OutlinerTreeModel::data(const QModelIndex &index, int role) const
{
    const OutlinerNode *node = nodeFromIndex(index);
    if (node == nullptr || node->layer_index < 0 || node->layer_index >= m_document_data.layers.size())
    {
        return QVariant();
    }

    const LayerData &layer = m_document_data.layers.at(node->layer_index);
    const bool isLayer = node->kind == OutlinerNodeKind::Layer;
    if (!isLayer
        && (node->primitive_index < 0 || node->primitive_index >= layer.primitives.size()))
    {
        return QVariant();
    }

    const QString text = isLayer ? layerText(layer) : layer.primitives.at(node->primitive_index).display_name;

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case OutlinerFilterTextRole:
        return text;
    case Qt::ToolTipRole:
        return isLayer ? layerToolTip(layer) : text;
    case Qt::CheckStateRole:
        return isLayer ? QVariant::fromValue(layerCheckState(layer))
                       : QVariant::fromValue(layer.primitives.at(node->primitive_index).visible ? Qt::Checked
                                                                                                 : Qt::Unchecked);
    case OutlinerNodeKindRole:
        return static_cast<int>(node->kind);
    case OutlinerLayerIndexRole:
        return node->layer_index;
    case OutlinerPrimitiveIndexRole:
        return node->primitive_index;
    case OutlinerIconNameRole:
        return isLayer ? layerIconName(layer) : primitiveIconName(layer, node->primitive_index);
    case OutlinerHiddenRole:
        return isLayer ? layerCheckState(layer) == Qt::Unchecked : !layer.primitives.at(node->primitive_index).visible;
    default:
        return QVariant();
    }
}

bool OutlinerTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    OutlinerNode *node = nodeFromIndex(index);
    if (node == nullptr || role != Qt::CheckStateRole || node->layer_index < 0
        || node->layer_index >= m_document_data.layers.size())
    {
        return false;
    }

    const bool visible = value.value<Qt::CheckState>() == Qt::Checked;
    LayerData &layer = m_document_data.layers[node->layer_index];

    if (node->kind == OutlinerNodeKind::Layer)
    {
        for (LayerPrimitiveData &primitive : layer.primitives)
        {
            primitive.visible = visible;
        }
        layer.visible = visible;
        emitAllRowsChanged();
        emit layerVisibilityChangeRequested(node->layer_index, visible);
        return true;
    }

    if (node->primitive_index < 0 || node->primitive_index >= layer.primitives.size())
    {
        return false;
    }

    layer.primitives[node->primitive_index].visible = visible;
    layer.visible = std::any_of(
        layer.primitives.cbegin(),
        layer.primitives.cend(),
        [](const LayerPrimitiveData &primitive) {
            return primitive.visible;
        });

    emit dataChanged(index, index, {Qt::CheckStateRole, OutlinerHiddenRole});
    const QModelIndex parentIndex = parent(index);
    if (parentIndex.isValid())
    {
        emit dataChanged(parentIndex, parentIndex, {Qt::CheckStateRole, OutlinerHiddenRole});
    }

    emit primitiveVisibilityChangeRequested(node->layer_index, node->primitive_index, visible);
    return true;
}

Qt::ItemFlags OutlinerTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

void OutlinerTreeModel::setDocumentData(const DocumentData &documentData, bool rebuildTreeItems)
{
    if (rebuildTreeItems || !hasSameShape(documentData))
    {
        beginResetModel();
        m_document_data = documentData;
        rebuildTree();
        endResetModel();
        return;
    }

    m_document_data = documentData;
    emitAllRowsChanged();
}

QModelIndex OutlinerTreeModel::indexForSelection(const SelectionState &selectionState) const
{
    if (selectionState.kind == SelectionKind::None || selectionState.layer_index < 0
        || selectionState.layer_index >= static_cast<int>(m_root_node->children.size()))
    {
        return QModelIndex();
    }

    const OutlinerNode *layerNode = m_root_node->children.at(selectionState.layer_index).get();
    if (selectionState.kind == SelectionKind::Layer)
    {
        return indexForNode(layerNode);
    }

    if (selectionState.primitive_index < 0
        || selectionState.primitive_index >= static_cast<int>(layerNode->children.size()))
    {
        return QModelIndex();
    }

    return indexForNode(layerNode->children.at(selectionState.primitive_index).get());
}

SelectionState OutlinerTreeModel::selectionForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return SelectionState {};
    }

    const OutlinerNode *node = nodeFromIndex(index);
    if (node == nullptr || node == m_root_node.get())
    {
        return SelectionState {};
    }

    return SelectionState {
        node->kind == OutlinerNodeKind::Layer ? SelectionKind::Layer : SelectionKind::Primitive,
        node->layer_index,
        node->primitive_index
    };
}

int OutlinerTreeModel::layerCount() const
{
    return m_document_data.layers.size();
}

int OutlinerTreeModel::visiblePrimitiveCount() const
{
    int visibleCount = 0;
    for (const LayerData &layer : m_document_data.layers)
    {
        for (const LayerPrimitiveData &primitive : layer.primitives)
        {
            if (primitive.visible)
            {
                ++visibleCount;
            }
        }
    }

    return visibleCount;
}

OutlinerTreeModel::OutlinerNode *OutlinerTreeModel::nodeFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return m_root_node.get();
    }

    return static_cast<OutlinerNode *>(index.internalPointer());
}

QModelIndex OutlinerTreeModel::indexForNode(const OutlinerNode *node) const
{
    if (node == nullptr || node == m_root_node.get())
    {
        return QModelIndex();
    }

    return createIndex(node->row, 0, const_cast<OutlinerNode *>(node));
}

bool OutlinerTreeModel::hasSameShape(const DocumentData &documentData) const
{
    if (documentData.layers.size() != m_document_data.layers.size())
    {
        return false;
    }

    for (int layerIndex = 0; layerIndex < documentData.layers.size(); ++layerIndex)
    {
        if (documentData.layers.at(layerIndex).primitives.size()
            != m_document_data.layers.at(layerIndex).primitives.size())
        {
            return false;
        }
    }

    return true;
}

void OutlinerTreeModel::rebuildTree()
{
    m_root_node = std::make_unique<OutlinerNode>();
    for (int layerIndex = 0; layerIndex < m_document_data.layers.size(); ++layerIndex)
    {
        auto layerNode = std::make_unique<OutlinerNode>();
        layerNode->kind = OutlinerNodeKind::Layer;
        layerNode->row = layerIndex;
        layerNode->layer_index = layerIndex;
        layerNode->primitive_index = -1;
        layerNode->parent = m_root_node.get();

        const LayerData &layer = m_document_data.layers.at(layerIndex);
        for (int primitiveIndex = 0; primitiveIndex < layer.primitives.size(); ++primitiveIndex)
        {
            auto primitiveNode = std::make_unique<OutlinerNode>();
            primitiveNode->kind = OutlinerNodeKind::Primitive;
            primitiveNode->row = primitiveIndex;
            primitiveNode->layer_index = layerIndex;
            primitiveNode->primitive_index = primitiveIndex;
            primitiveNode->parent = layerNode.get();
            layerNode->children.push_back(std::move(primitiveNode));
        }

        m_root_node->children.push_back(std::move(layerNode));
    }
}

void OutlinerTreeModel::emitAllRowsChanged()
{
    for (int layerIndex = 0; layerIndex < static_cast<int>(m_root_node->children.size()); ++layerIndex)
    {
        const QModelIndex layerIndexModel = indexForNode(m_root_node->children.at(layerIndex).get());
        emit dataChanged(
            layerIndexModel,
            layerIndexModel,
            {Qt::DisplayRole, Qt::ToolTipRole, Qt::CheckStateRole, OutlinerIconNameRole, OutlinerHiddenRole});

        const OutlinerNode *layerNode = m_root_node->children.at(layerIndex).get();
        for (int primitiveIndex = 0; primitiveIndex < static_cast<int>(layerNode->children.size()); ++primitiveIndex)
        {
            const QModelIndex primitiveIndexModel = indexForNode(layerNode->children.at(primitiveIndex).get());
            emit dataChanged(
                primitiveIndexModel,
                primitiveIndexModel,
                {Qt::DisplayRole, Qt::ToolTipRole, Qt::CheckStateRole, OutlinerIconNameRole, OutlinerHiddenRole});
        }
    }
}

QString OutlinerTreeModel::layerText(const LayerData &layer) const
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

QString OutlinerTreeModel::layerToolTip(const LayerData &layer) const
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

QString OutlinerTreeModel::layerIconName(const LayerData &layer) const
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

QString OutlinerTreeModel::primitiveIconName(const LayerData &layer, int primitiveIndex) const
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

Qt::CheckState OutlinerTreeModel::layerCheckState(const LayerData &layer) const
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

} // namespace PolyShow
