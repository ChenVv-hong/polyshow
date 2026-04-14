#pragma once

#include "core/GeometryScene.h"

#include <QMainWindow>
#include <QPointF>

class QAction;
class QActionGroup;
class QComboBox;
class QLabel;
class QSplitter;
class QToolBar;

namespace PolyShow
{

class GeometryViewer;
class PropertyPanel;

/// Main application window for the PolyShow viewer.
class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    /// Creates the main window and initializes the UI.
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    /// Opens and parses a `.ply` file chosen by the user.
    void openPlyFile();

    /// Switches to solid render mode.
    void setRenderModeSolid();

    /// Switches to wireframe render mode.
    void setRenderModeWireframe();

    /// Switches to point render mode.
    void setRenderModePoints();

    /// Updates the mouse coordinate text in the status bar.
    void updateMousePosition(const QPointF &scenePosition);

    /// Refreshes the displayed geometry statistics.
    void onGeometryChanged(int pointCount, int polylineCount, int polygonCount);

    /// Maps combo-box selection changes to render modes.
    void onRenderModeChanged(int index);

    /// Shows the About dialog.
    void showAboutDialog();

private:
    /// Builds the core widgets and layout tree.
    void setupUi();

    /// Creates the menu bar and menu actions.
    void setupMenuBar();

    /// Creates the toolbar and render mode selector.
    void setupToolBar();

    /// Initializes the status bar widgets.
    void setupStatusBar();

    /// Synchronizes a render mode to the full UI state.
    void setRenderMode(GeometryScene::RenderMode renderMode);

    /// Refreshes UI state from the current scene data.
    void updateUiFromScene();

    GeometryScene *m_scene {nullptr};
    GeometryViewer *m_geometry_viewer {nullptr};
    PropertyPanel *m_property_panel {nullptr};
    QSplitter *m_splitter {nullptr};

    QToolBar *m_tool_bar {nullptr};
    QComboBox *m_render_mode_combo_box {nullptr};

    QAction *m_open_action {nullptr};
    QAction *m_exit_action {nullptr};
    QAction *m_fit_action {nullptr};
    QAction *m_zoom_in_action {nullptr};
    QAction *m_zoom_out_action {nullptr};
    QAction *m_reset_view_action {nullptr};
    QAction *m_about_action {nullptr};

    QAction *m_solid_mode_action {nullptr};
    QAction *m_wireframe_mode_action {nullptr};
    QAction *m_points_mode_action {nullptr};
    QActionGroup *m_render_mode_action_group {nullptr};

    QLabel *m_status_info_label {nullptr};
    QLabel *m_status_mouse_label {nullptr};
};

} // namespace PolyShow
