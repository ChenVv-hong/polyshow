#pragma once

#include "core/GeometryTypes.h"

#include <QPersistentModelIndex>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTreeView;
class QWidget;

namespace PolyShow
{

class OutlinerFilterProxyModel;
class OutlinerTreeModel;


/// Displays the imported file tree, visibility controls, and search field.
class LayerSidebar final : public QWidget
{
    Q_OBJECT

public:
    /// Creates the layer sidebar widget.
    explicit LayerSidebar(QWidget *parent = nullptr);

    /// Rebuilds the sidebar from the current document.
    void setDocumentData(const DocumentData &documentData, bool rebuildTreeItems = true);

    /// Synchronizes the current layer or primitive selection.
    void setSelectionState(const SelectionState &selectionState);

signals:
    /// Emitted when the selected object changes.
    void selectionChanged(const SelectionState &selectionState);

    /// Emitted when the user toggles one layer.
    void layerVisibilityChanged(int layerIndex, bool visible);

    /// Emitted when the user toggles one primitive.
    void primitiveVisibilityChanged(int layerIndex, int primitiveIndex, bool visible);

    /// Emitted when the user requests a new layer.
    void createLayerRequested();

    /// Emitted when the user requests exporting the active layer.
    void exportLayerRequested();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;

    /// Expands or collapses the search field.
    void setSearchExpanded(bool expanded);

    /// Updates the sidebar footer summary.
    void updateFooter();

    /// Applies the current filter text to the tree.
    void applyFilter(bool expandVisibleLayersAfterFilter);

    /// Keeps the tree/empty surfaces aligned with the current document state.
    void updateContentSurface();

    /// Expands all visible layer rows after model or filter changes.
    void expandVisibleLayers();

    DocumentData m_document_data;
    SelectionState m_selection_state;
    QPushButton *m_new_layer_button {nullptr};
    QPushButton *m_export_layer_button {nullptr};
    QPushButton *m_search_button {nullptr};
    QLineEdit *m_search_line_edit {nullptr};
    QWidget *m_empty_widget {nullptr};
    QLabel *m_footer_label {nullptr};
    QTreeView *m_tree_view {nullptr};
    OutlinerTreeModel *m_tree_model {nullptr};
    OutlinerFilterProxyModel *m_filter_model {nullptr};
    bool m_is_search_expanded {false};
    bool m_is_syncing_selection {false};
    bool m_is_applying_filter {false};
    bool m_has_tree_control_press {false};
    QPersistentModelIndex m_tree_control_press_index;
    int m_tree_control_press_target {0};
};

} // namespace PolyShow
