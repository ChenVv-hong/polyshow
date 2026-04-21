#pragma once

#include "core/PrimitiveEditing.h"
#include "core/GeometryScene.h"
#include "ui/UiTheme.h"

#include <QMainWindow>
#include <QPointF>
#include <QStringList>

class QAction;
class QActionGroup;
class QComboBox;
class QLabel;
class QSplitter;
class QWidget;

namespace PolyShow
{

class GeometryViewer;
class InspectorPanel;
class LayerSidebar;
class LogPanel;
class PanelFrame;
class PillButton;

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

    /// Imports one or more `.ply` files as additional layers.
    void importPlyFiles();

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

    /// Applies a layer visibility toggle from the property panel.
    void onLayerVisibilityChanged(int layerIndex, bool visible);

    /// Applies a primitive visibility toggle from the property panel.
    void onPrimitiveVisibilityChanged(int layerIndex, int primitiveIndex, bool visible);

    /// Maps combo-box selection changes to render modes.
    void onRenderModeChanged(int index);

    /// Applies the latest selection state to the full UI.
    void onSelectionStateChanged(const SelectionState &selectionState);

    /// Handles a primitive click coming from the scene view.
    void onScenePrimitiveActivated(int layerIndex, int primitiveIndex);

    /// Handles a click on empty scene space.
    void onEmptySceneActivated();

    /// Validates and applies one field-level style change from the inspector.
    void onInspectorStyleChangeRequested(const PrimitiveStyleChangeRequest &request);

    /// Validates one real-time coordinate draft from the inspector.
    void onInspectorCoordinateDraftChanged(const PrimitiveCoordinateDraft &draft);

    /// Shows the About dialog.
    void showAboutDialog();

private:
    /// Builds the core widgets and layout tree.
    void setupUi();

    /// Creates the menu bar and menu actions.
    void setupMenuBar();

    /// Initializes the status bar widgets.
    void setupStatusBar();

    /// Creates the in-viewport tool controls.
    void setupViewportControls();

    /// Synchronizes viewport buttons and selectors to scene state.
    void updateViewportControlState();

    /// Synchronizes a render mode to the full UI state.
    void setRenderMode(GeometryScene::RenderMode renderMode);

    /// Imports files into the current document, optionally replacing it first.
    void importFiles(const QStringList &filePaths, bool replaceExisting);

    /// Pushes the current document state into the scene and side panel.
    void syncDocumentToViews(bool fitScene);

    /// Refreshes visibility-dependent UI without rebuilding the sidebar tree.
    void refreshViewsForVisibilityChange();

    /// Reloads the inspector after one selection-context change.
    void reloadInspectorForSelectionChange();

    /// Refreshes the scene after one primitive edit without touching other views.
    void refreshSceneForPrimitiveEdit();

    /// Clears the current coordinate preview suppression state.
    void clearCoordinatePreviewState();

    /// Normalizes one selection state against the current document.
    [[nodiscard]]
    SelectionState normalizedSelectionState(const SelectionState &selectionState) const;

    /// Refreshes UI state from the current scene data.
    void updateUiFromScene();

    GeometryScene *m_scene {nullptr};
    GeometryViewer *m_geometry_viewer {nullptr};
    LayerSidebar *m_layer_sidebar {nullptr};
    InspectorPanel *m_inspector_panel {nullptr};
    LogPanel *m_log_panel {nullptr};
    QSplitter *m_splitter {nullptr};
    QSplitter *m_vertical_splitter {nullptr};
    PanelFrame *m_viewport_frame {nullptr};
    PanelFrame *m_inspector_container {nullptr};
    PanelFrame *m_log_panel_container {nullptr};
    QWidget *m_viewport_controls_widget {nullptr};
    QComboBox *m_render_mode_combo_box {nullptr};
    PillButton *m_grid_toggle_button {nullptr};

    QAction *m_open_action {nullptr};
    QAction *m_import_action {nullptr};
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

    DocumentData m_document_data;
    SelectionState m_selection_state;
    PrimitiveEditPreviewState m_edit_preview_state;
    bool m_has_logged_invalid_coordinate_draft {false};
    ThemeMode m_theme_mode {ThemeMode::Light};
};

} // namespace PolyShow
