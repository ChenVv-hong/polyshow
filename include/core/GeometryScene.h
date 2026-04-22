#pragma once

#include "core/PrimitiveEditing.h"
#include "core/GeometryTypes.h"

#include <QGraphicsScene>

namespace PolyShow
{

/// Builds drawable scene items from imported document data.
class GeometryScene final : public QGraphicsScene
{
    Q_OBJECT

public:
    /// Supported scene render modes.
    enum class RenderMode
    {
        Solid,
        Wireframe,
        Points
    };
    Q_ENUM(RenderMode)

    /// Creates the scene and initializes the default grid.
    explicit GeometryScene(QObject *parent = nullptr);

    /// Replaces the current document payload.
    void setDocumentData(const DocumentData &documentData);

    /// Clears the current document payload.
    void clearDocument();

    /// Changes the active render mode.
    void setRenderMode(RenderMode renderMode);

    /// Returns the active render mode.
    [[nodiscard]]
    RenderMode renderMode() const;

    /// Shows or hides the background grid.
    void setGridVisible(bool visible);

    /// Returns whether the background grid is visible.
    [[nodiscard]]
    bool isGridVisible() const;

    /// Returns the standalone point count.
    [[nodiscard]]
    int pointCount() const;

    /// Returns the polyline count.
    [[nodiscard]]
    int polylineCount() const;

    /// Returns the polygon count.
    [[nodiscard]]
    int polygonCount() const;

    /// Returns the current selection state.
    [[nodiscard]]
    SelectionState selectionState() const;

    /// Updates the selected layer or primitive.
    void setSelectionState(const SelectionState &selectionState);

    /// Replaces the active inspector preview suppression state.
    void setEditPreviewState(const PrimitiveEditPreviewState &previewState, bool rebuildScene = true);

signals:
    /// Emitted after the geometry statistics change.
    void geometryChanged(int pointCount, int polylineCount, int polygonCount);

    /// Emitted after the selected object changes.
    void selectionStateChanged(const SelectionState &selectionState);

private:
    /// Recomputes the cached visible primitive counts.
    void updateVisibleCounts();

    /// Rebuilds all scene items from the stored state.
    void rebuildScene();

    DocumentData m_document_data;
    RenderMode m_render_mode {RenderMode::Solid};
    bool m_is_grid_visible {true};
    int m_point_count {0};
    int m_polyline_count {0};
    int m_polygon_count {0};
    SelectionState m_selection_state;
    PrimitiveEditPreviewState m_edit_preview_state;
};

} // namespace PolyShow
