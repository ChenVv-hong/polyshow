#pragma once

#include "core/GeometryScene.h"

#include <QString>
#include <QWidget>

class QCheckBox;
class QLabel;
class QTreeWidget;

namespace PolyShow
{

/// Displays document, statistics, and visibility controls for the current scene.
class PropertyPanel final : public QWidget
{
    Q_OBJECT

public:
    /// Creates the property panel widget.
    explicit PropertyPanel(QWidget *parent = nullptr);

    /// Rebuilds the document summary and layer tree.
    void setDocumentData(const DocumentData &documentData);

    /// Updates the geometry counters.
    void setGeometryStats(int points, int polylines, int polygons);

    /// Updates the render mode text.
    void setRenderMode(GeometryScene::RenderMode renderMode);

signals:
    /// Emitted when the user toggles the grid checkbox.
    void gridVisibilityChanged(bool visible);

    /// Emitted when the user toggles one layer.
    void layerVisibilityChanged(int layerIndex, bool visible);

    /// Emitted when the user toggles one primitive inside a layer.
    void primitiveVisibilityChanged(int layerIndex, int primitiveIndex, bool visible);

private:
    /// Updates the compact loaded-files summary.
    void updateLoadedFilesSummary(const DocumentData &documentData);

    /// Rebuilds the layer and primitive visibility tree.
    void rebuildLayerTree(const DocumentData &documentData);

    QLabel *m_file_value_label {nullptr};
    QLabel *m_points_value_label {nullptr};
    QLabel *m_polylines_value_label {nullptr};
    QLabel *m_polygons_value_label {nullptr};
    QLabel *m_render_mode_value_label {nullptr};
    QCheckBox *m_grid_check_box {nullptr};
    QTreeWidget *m_layer_tree_widget {nullptr};
};

} // namespace PolyShow
