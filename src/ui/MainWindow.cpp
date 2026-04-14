#include "ui/MainWindow.h"

#include "parsers/PlyParser.h"
#include "ui/GeometryViewer.h"
#include "ui/PropertyPanel.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QFileDialog>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

namespace PolyShow
{

/// Creates the main window and initializes the full UI tree.
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    updateUiFromScene();
}

/// Opens a file, parses it, and syncs the result to the scene widgets.
void MainWindow::openPlyFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open PolyShow .ply File"),
        QString(),
        QStringLiteral("PolyShow Files (*.ply);;All Files (*.*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    GeometryData data;
    QString errorMessage;
    if (!PlyParser::parseFile(filePath, data, &errorMessage))
    {
        QMessageBox::critical(this, QStringLiteral("Load Failed"), errorMessage);
        statusBar()->showMessage(QStringLiteral("Load failed"), 3000);
        return;
    }

    // Keep the scene, panel, and viewport in sync after a successful parse.
    m_scene->setGeometryData(data);
    m_property_panel->setCurrentFile(filePath);
    m_geometry_viewer->fitScene();
    statusBar()->showMessage(QStringLiteral("File loaded"), 2000);
}

/// Switches the UI to solid render mode.
void MainWindow::setRenderModeSolid()
{
    setRenderMode(GeometryScene::RenderMode::Solid);
}

/// Switches the UI to wireframe render mode.
void MainWindow::setRenderModeWireframe()
{
    setRenderMode(GeometryScene::RenderMode::Wireframe);
}

/// Switches the UI to point render mode.
void MainWindow::setRenderModePoints()
{
    setRenderMode(GeometryScene::RenderMode::Points);
}

/// Writes the latest mouse position into the status bar.
void MainWindow::updateMousePosition(const QPointF &scenePosition)
{
    m_status_mouse_label->setText(
        QStringLiteral("X: %1  Y: %2").arg(scenePosition.x(), 0, 'f', 2).arg(scenePosition.y(), 0, 'f', 2));
}

/// Refreshes geometry counters in the panel and status bar.
void MainWindow::onGeometryChanged(int pointCount, int polylineCount, int polygonCount)
{
    m_property_panel->setGeometryStats(pointCount, polylineCount, polygonCount);
    m_status_info_label->setText(
        QStringLiteral("Points: %1  Polylines: %2  Polygons: %3").arg(pointCount).arg(polylineCount).arg(polygonCount));
}

/// Maps the combo-box index to an internal render mode.
void MainWindow::onRenderModeChanged(int index)
{
    switch (index)
    {
    case 0:
        setRenderMode(GeometryScene::RenderMode::Solid);
        break;
    case 1:
        setRenderMode(GeometryScene::RenderMode::Wireframe);
        break;
    case 2:
        setRenderMode(GeometryScene::RenderMode::Points);
        break;
    default:
        break;
    }
}

/// Shows the application overview dialog.
void MainWindow::showAboutDialog()
{
    QMessageBox::about(
        this,
        QStringLiteral("About PolyShow"),
        QStringLiteral("PolyShow MVP\n\n") + QStringLiteral("Features:\n")
            + QStringLiteral("1. Open and render PolyShow .ply files\n")
            + QStringLiteral("2. Display points, polylines, and polygons\n")
            + QStringLiteral("3. Zoom, pan, fit, and reset view\n")
            + QStringLiteral("4. Switch render mode (Solid/Wireframe/Points)\n")
            + QStringLiteral("5. Show stats and live mouse coordinates"));
}

/// Builds the core widgets, layout, and signal connections.
void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("PolyShow"));
    resize(1280, 800);

    // The scene and viewer form the main geometry display area.
    m_scene = new GeometryScene(this);
    m_geometry_viewer = new GeometryViewer(this);
    m_geometry_viewer->setScene(m_scene);

    // Keep the property panel docked on the right so metadata stays visible.
    m_property_panel = new PropertyPanel(this);
    m_property_panel->setMinimumWidth(280);
    m_property_panel->setMaximumWidth(380);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_geometry_viewer);
    m_splitter->addWidget(m_property_panel);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 0);

    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);
    setCentralWidget(centralWidget);

    // Bridge scene and viewer updates into top-level window state.
    connect(m_scene, &GeometryScene::geometryChanged, this, &MainWindow::onGeometryChanged);
    connect(m_geometry_viewer, &GeometryViewer::mousePositionChanged, this, &MainWindow::updateMousePosition);
    connect(m_property_panel, &PropertyPanel::gridVisibilityChanged, m_scene, &GeometryScene::setGridVisible);
}

/// Creates all menu actions and attaches them to the menu bar.
void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("File"));
    m_open_action = new QAction(QStringLiteral("Open .ply..."), this);
    m_open_action->setShortcut(QKeySequence::Open);
    connect(m_open_action, &QAction::triggered, this, &MainWindow::openPlyFile);
    fileMenu->addAction(m_open_action);

    fileMenu->addSeparator();

    m_exit_action = new QAction(QStringLiteral("Exit"), this);
    m_exit_action->setShortcut(QKeySequence::Quit);
    connect(m_exit_action, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(m_exit_action);

    auto *viewMenu = menuBar()->addMenu(QStringLiteral("View"));

    m_fit_action = new QAction(QStringLiteral("Fit to View"), this);
    connect(m_fit_action, &QAction::triggered, m_geometry_viewer, &GeometryViewer::fitScene);
    viewMenu->addAction(m_fit_action);

    m_zoom_in_action = new QAction(QStringLiteral("Zoom In"), this);
    m_zoom_in_action->setShortcut(QKeySequence::ZoomIn);
    connect(m_zoom_in_action, &QAction::triggered, m_geometry_viewer, &GeometryViewer::zoomIn);
    viewMenu->addAction(m_zoom_in_action);

    m_zoom_out_action = new QAction(QStringLiteral("Zoom Out"), this);
    m_zoom_out_action->setShortcut(QKeySequence::ZoomOut);
    connect(m_zoom_out_action, &QAction::triggered, m_geometry_viewer, &GeometryViewer::zoomOut);
    viewMenu->addAction(m_zoom_out_action);

    m_reset_view_action = new QAction(QStringLiteral("Reset View"), this);
    connect(m_reset_view_action, &QAction::triggered, m_geometry_viewer, &GeometryViewer::resetViewTransform);
    viewMenu->addAction(m_reset_view_action);

    auto *renderMenu = menuBar()->addMenu(QStringLiteral("Render"));
    m_render_mode_action_group = new QActionGroup(this);
    m_render_mode_action_group->setExclusive(true);

    m_solid_mode_action = new QAction(QStringLiteral("Solid"), this);
    m_solid_mode_action->setCheckable(true);
    connect(m_solid_mode_action, &QAction::triggered, this, &MainWindow::setRenderModeSolid);
    m_render_mode_action_group->addAction(m_solid_mode_action);
    renderMenu->addAction(m_solid_mode_action);

    m_wireframe_mode_action = new QAction(QStringLiteral("Wireframe"), this);
    m_wireframe_mode_action->setCheckable(true);
    connect(m_wireframe_mode_action, &QAction::triggered, this, &MainWindow::setRenderModeWireframe);
    m_render_mode_action_group->addAction(m_wireframe_mode_action);
    renderMenu->addAction(m_wireframe_mode_action);

    m_points_mode_action = new QAction(QStringLiteral("Points"), this);
    m_points_mode_action->setCheckable(true);
    connect(m_points_mode_action, &QAction::triggered, this, &MainWindow::setRenderModePoints);
    m_render_mode_action_group->addAction(m_points_mode_action);
    renderMenu->addAction(m_points_mode_action);

    auto *helpMenu = menuBar()->addMenu(QStringLiteral("Help"));
    m_about_action = new QAction(QStringLiteral("About"), this);
    connect(m_about_action, &QAction::triggered, this, &MainWindow::showAboutDialog);
    helpMenu->addAction(m_about_action);
}

/// Creates the toolbar and the render mode combo box.
void MainWindow::setupToolBar()
{
    m_tool_bar = addToolBar(QStringLiteral("Main Toolbar"));
    m_tool_bar->setMovable(false);

    m_tool_bar->addAction(m_open_action);
    m_tool_bar->addSeparator();
    m_tool_bar->addAction(m_fit_action);
    m_tool_bar->addAction(m_zoom_in_action);
    m_tool_bar->addAction(m_zoom_out_action);
    m_tool_bar->addAction(m_reset_view_action);
    m_tool_bar->addSeparator();

    // Keep the render mode selector visible for quick mode switching.
    auto *renderModeLabel = new QLabel(QStringLiteral("Render mode: "), this);
    m_tool_bar->addWidget(renderModeLabel);

    m_render_mode_combo_box = new QComboBox(this);
    m_render_mode_combo_box->addItem(QStringLiteral("Solid"));
    m_render_mode_combo_box->addItem(QStringLiteral("Wireframe"));
    m_render_mode_combo_box->addItem(QStringLiteral("Points"));
    connect(
        m_render_mode_combo_box,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &MainWindow::onRenderModeChanged);
    m_tool_bar->addWidget(m_render_mode_combo_box);
}

/// Creates the permanent status bar widgets.
void MainWindow::setupStatusBar()
{
    m_status_info_label = new QLabel(QStringLiteral("Points: 0  Polylines: 0  Polygons: 0"), this);
    m_status_mouse_label = new QLabel(QStringLiteral("X: 0.00  Y: 0.00"), this);

    statusBar()->addPermanentWidget(m_status_info_label);
    statusBar()->addPermanentWidget(m_status_mouse_label);
    statusBar()->showMessage(QStringLiteral("Ready"), 1500);
}

/// Synchronizes the chosen render mode to the scene and all UI controls.
void MainWindow::setRenderMode(GeometryScene::RenderMode renderMode)
{
    m_scene->setRenderMode(renderMode);
    m_property_panel->setRenderMode(renderMode);

    int comboIndex = 0;
    QAction *checkedAction = m_solid_mode_action;

    switch (renderMode)
    {
    case GeometryScene::RenderMode::Solid:
        comboIndex = 0;
        checkedAction = m_solid_mode_action;
        break;
    case GeometryScene::RenderMode::Wireframe:
        comboIndex = 1;
        checkedAction = m_wireframe_mode_action;
        break;
    case GeometryScene::RenderMode::Points:
        comboIndex = 2;
        checkedAction = m_points_mode_action;
        break;
    default:
        break;
    }

    checkedAction->setChecked(true);

    // Block the combo-box signal so programmatic sync does not trigger another mode switch.
    const QSignalBlocker blocker(m_render_mode_combo_box);
    m_render_mode_combo_box->setCurrentIndex(comboIndex);
}

/// Rehydrates the window state from the current scene object.
void MainWindow::updateUiFromScene()
{
    setRenderMode(m_scene->renderMode());
    onGeometryChanged(m_scene->pointCount(), m_scene->polylineCount(), m_scene->polygonCount());
}

} // namespace PolyShow
