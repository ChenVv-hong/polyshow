#pragma once

#include "core/GeometryTypes.h"

#include <QAbstractItemModel>

#include <memory>
#include <vector>

namespace PolyShow
{

enum class OutlinerNodeKind
{
    Layer = 1,
    Primitive = 2
};

enum OutlinerItemRole
{
    OutlinerNodeKindRole = Qt::UserRole + 1,
    OutlinerLayerIndexRole,
    OutlinerPrimitiveIndexRole,
    OutlinerIconNameRole,
    OutlinerFilterTextRole,
    OutlinerHiddenRole
};

/// Tree model backing the left outliner layer/primitive hierarchy.
class OutlinerTreeModel final : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit OutlinerTreeModel(QObject *parent = nullptr);

    [[nodiscard]]
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]]
    QModelIndex parent(const QModelIndex &child) const override;

    [[nodiscard]]
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]]
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]]
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    [[nodiscard]]
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /// Replaces or synchronizes the document payload represented by the model.
    void setDocumentData(const DocumentData &documentData, bool rebuildTreeItems);

    /// Returns the source-model index for the current application selection.
    [[nodiscard]]
    QModelIndex indexForSelection(const SelectionState &selectionState) const;

    /// Converts one source-model index to the application selection payload.
    [[nodiscard]]
    SelectionState selectionForIndex(const QModelIndex &index) const;

    [[nodiscard]]
    int layerCount() const;

    [[nodiscard]]
    int visiblePrimitiveCount() const;

signals:
    void layerVisibilityChangeRequested(int layerIndex, bool visible);
    void primitiveVisibilityChangeRequested(int layerIndex, int primitiveIndex, bool visible);

private:
    struct OutlinerNode
    {
        OutlinerNodeKind kind {OutlinerNodeKind::Layer};
        int row {0};
        int layer_index {-1};
        int primitive_index {-1};
        OutlinerNode *parent {nullptr};
        std::vector<std::unique_ptr<OutlinerNode>> children;
    };

    [[nodiscard]]
    OutlinerNode *nodeFromIndex(const QModelIndex &index) const;

    [[nodiscard]]
    QModelIndex indexForNode(const OutlinerNode *node) const;

    [[nodiscard]]
    bool hasSameShape(const DocumentData &documentData) const;

    void rebuildTree();
    void emitAllRowsChanged();

    [[nodiscard]]
    QString layerText(const LayerData &layer) const;

    [[nodiscard]]
    QString layerToolTip(const LayerData &layer) const;

    [[nodiscard]]
    QString layerIconName(const LayerData &layer) const;

    [[nodiscard]]
    QString primitiveIconName(const LayerData &layer, int primitiveIndex) const;

    [[nodiscard]]
    Qt::CheckState layerCheckState(const LayerData &layer) const;

    DocumentData m_document_data;
    std::unique_ptr<OutlinerNode> m_root_node;
};

} // namespace PolyShow
