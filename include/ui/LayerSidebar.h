#pragma once

#include "core/GeometryTypes.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace PolyShow
{

/// Displays the imported file tree, visibility controls, and search field.
class LayerSidebar final : public QWidget
{
    Q_OBJECT

public:
    /// Creates the layer sidebar widget.
    explicit LayerSidebar(QWidget *parent = nullptr);

    /// Rebuilds the sidebar from the current document.
    void setDocumentData(const DocumentData &documentData);

    /// Synchronizes the current layer or primitive selection.
    void setSelectionState(const SelectionState &selectionState);

signals:
    /// Emitted when the selected object changes.
    void selectionChanged(const SelectionState &selectionState);

    /// Emitted when the user toggles one layer.
    void layerVisibilityChanged(int layerIndex, bool visible);

    /// Emitted when the user toggles one primitive.
    void primitiveVisibilityChanged(int layerIndex, int primitiveIndex, bool visible);

private:
    /// Expands or collapses the search field.
    void setSearchExpanded(bool expanded);

    /// Updates the sidebar footer summary.
    void updateFooter();

    /// Rebuilds the tree widget content.
    void rebuildTree();

    /// Applies the current filter text to the tree.
    void applyFilter();

    /// Returns the item representing the given selection state, if any.
    [[nodiscard]]
    QTreeWidgetItem *findItem(const SelectionState &selectionState) const;

    DocumentData m_document_data;
    SelectionState m_selection_state;
    QPushButton *m_search_button {nullptr};
    QLineEdit *m_search_line_edit {nullptr};
    QLabel *m_section_label {nullptr};
    QLabel *m_footer_label {nullptr};
    QTreeWidget *m_tree_widget {nullptr};
    bool m_is_search_expanded {false};
};

} // namespace PolyShow
