#pragma once

#include "core/GeometryScene.h"

#include <QString>
#include <QWidget>

class QCheckBox;
class QLabel;

namespace PolyShow
{

/// Displays file, statistics, and view options for the current scene.
class PropertyPanel final : public QWidget
{
    Q_OBJECT

public:
    /// Creates the property panel widget.
    explicit PropertyPanel(QWidget *parent = nullptr);

    /// Updates the current file path display.
    void setCurrentFile(const QString &filePath);

    /// Updates the geometry counters.
    void setGeometryStats(int points, int polylines, int polygons);

    /// Updates the render mode text.
    void setRenderMode(GeometryScene::RenderMode renderMode);

signals:
    /// Emitted when the user toggles the grid checkbox.
    void gridVisibilityChanged(bool visible);

private:
    QLabel *m_file_value_label {nullptr};
    QLabel *m_points_value_label {nullptr};
    QLabel *m_polylines_value_label {nullptr};
    QLabel *m_polygons_value_label {nullptr};
    QLabel *m_render_mode_value_label {nullptr};
    QCheckBox *m_grid_check_box {nullptr};
};

} // namespace PolyShow
