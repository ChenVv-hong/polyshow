#pragma once

#include "core/GeometryTypes.h"

#include <QGraphicsScene>

namespace PolyShow
{

/// Builds drawable scene items from parsed geometry data.
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

    /// Replaces the current geometry payload.
    void setGeometryData(const GeometryData &geometryData);

    /// Clears the current geometry payload.
    void clearGeometry();

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

signals:
    /// Emitted after the geometry statistics change.
    void geometryChanged(int pointCount, int polylineCount, int polygonCount);

private:
    /// Rebuilds all scene items from the stored state.
    void rebuildScene();

    /// Rebuilds the background grid.
    void rebuildGrid();

    GeometryData m_geometry_data;
    RenderMode m_render_mode {RenderMode::Solid};
    bool m_is_grid_visible {true};
};

} // namespace PolyShow
