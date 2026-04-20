#include "ui/MainWindow.h"

#include "parsers/PlyParser.h"
#include "ui/GeometryViewer.h"
#include "ui/InspectorPanel.h"
#include "ui/LayerSidebar.h"
#include "ui/LogPanel.h"
#include "ui/PanelFrame.h"
#include "ui/PillButton.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

namespace PolyShow
{

namespace
{

/// Returns the shared file dialog filter for PolyShow files.
QString plyFileDialogFilter()
{
    return QStringLiteral("PolyShow Files (*.ply);;All Files (*.*)");
}

/// Returns the visible label for one primitive entry.
QString primitiveDisplayName(PrimitiveKind kind, int ordinal)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        return QStringLiteral("Point %1").arg(ordinal);
    case PrimitiveKind::Polyline:
        return QStringLiteral("Polyline %1").arg(ordinal);
    case PrimitiveKind::Polygon:
        return QStringLiteral("Polygon %1").arg(ordinal);
    default:
        return QStringLiteral("Primitive %1").arg(ordinal);
    }
}

/// Appends one primitive entry to a layer list.
void appendPrimitiveEntry(
    QVector<LayerPrimitiveData> &primitives, PrimitiveKind kind, int index, int &ordinalCounter)
{
    ++ordinalCounter;
    primitives.append(
        LayerPrimitiveData {PrimitiveReference {kind, index}, primitiveDisplayName(kind, ordinalCounter), true});
}

/// Builds the runtime layer state for one successfully parsed file.
LayerData buildLayerData(const QString &filePath, const GeometryData &geometryData)
{
    LayerData layer;
    layer.file_path = filePath;
    layer.display_name = QFileInfo(filePath).fileName();
    layer.geometry = geometryData;

    int pointOrdinal = 0;
    int polylineOrdinal = 0;
    int polygonOrdinal = 0;

    if (!geometryData.primitive_order.isEmpty())
    {
        for (const PrimitiveReference &reference : geometryData.primitive_order)
        {
            switch (reference.kind)
            {
            case PrimitiveKind::Point:
                appendPrimitiveEntry(layer.primitives, reference.kind, reference.index, pointOrdinal);
                break;
            case PrimitiveKind::Polyline:
                appendPrimitiveEntry(layer.primitives, reference.kind, reference.index, polylineOrdinal);
                break;
            case PrimitiveKind::Polygon:
                appendPrimitiveEntry(layer.primitives, reference.kind, reference.index, polygonOrdinal);
                break;
            }
        }

        return layer;
    }

    for (int index = 0; index < geometryData.points.size(); ++index)
    {
        appendPrimitiveEntry(layer.primitives, PrimitiveKind::Point, index, pointOrdinal);
    }

    for (int index = 0; index < geometryData.polylines.size(); ++index)
    {
        appendPrimitiveEntry(layer.primitives, PrimitiveKind::Polyline, index, polylineOrdinal);
    }

    for (int index = 0; index < geometryData.polygons.size(); ++index)
    {
        appendPrimitiveEntry(layer.primitives, PrimitiveKind::Polygon, index, polygonOrdinal);
    }

    return layer;
}

/// Returns one selection state normalized against the current document.
SelectionState normalizedSelection(const DocumentData &documentData, const SelectionState &selectionState)
{
    if (selectionState.kind == SelectionKind::None)
    {
        return {};
    }

    if (selectionState.layer_index < 0 || selectionState.layer_index >= documentData.layers.size())
    {
        return {};
    }

    if (selectionState.kind == SelectionKind::Layer)
    {
        if (!documentData.layers.at(selectionState.layer_index).visible)
        {
            return {};
        }
        return selectionState;
    }

    const LayerData &layer = documentData.layers.at(selectionState.layer_index);
    if (!layer.visible)
    {
        return {};
    }

    if (selectionState.primitive_index < 0 || selectionState.primitive_index >= layer.primitives.size())
    {
        return {};
    }

    if (!layer.primitives.at(selectionState.primitive_index).visible)
    {
        return {};
    }

    return selectionState;
}

/// Formats one numeric value for compact log output.
QString formatNumber(double value)
{
    return QString::number(value, 'f', 2);
}

/// Returns the user-facing log label for one selected primitive.
QString primitiveLogLabel(const DocumentData &documentData, int layerIndex, int primitiveIndex)
{
    if (layerIndex < 0 || layerIndex >= documentData.layers.size())
    {
        return QStringLiteral("selected primitive");
    }

    const LayerData &layer = documentData.layers.at(layerIndex);
    if (primitiveIndex < 0 || primitiveIndex >= layer.primitives.size())
    {
        return QStringLiteral("selected primitive");
    }

    return QStringLiteral("%1 in %2").arg(layer.primitives.at(primitiveIndex).display_name, layer.display_name);
}

/// Returns whether two point lists carry the same geometry.
bool pointsMatch(const QVector<Point2D> &lhs, const QVector<Point2D> &rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (int index = 0; index < lhs.size(); ++index)
    {
        if (!pointsEqual(lhs.at(index), rhs.at(index)))
        {
            return false;
        }
    }

    return true;
}

/// Builds a compact summary of one applied primitive edit.
QString primitiveEditSummary(
    PrimitiveKind kind, const PrimitiveEditValues &beforeValues, const PrimitiveEditValues &afterValues)
{
    QStringList changes;

    if (beforeValues.style.color != afterValues.style.color)
    {
        changes.append(
            QStringLiteral("stroke %1 -> %2")
                .arg(formatColorText(beforeValues.style.color), formatColorText(afterValues.style.color)));
    }

    if (kind == PrimitiveKind::Polygon)
    {
        if (beforeValues.style.fill_enabled != afterValues.style.fill_enabled)
        {
            changes.append(
                QStringLiteral("fill %1 -> %2")
                    .arg(beforeValues.style.fill_enabled ? QStringLiteral("on") : QStringLiteral("off"))
                    .arg(afterValues.style.fill_enabled ? QStringLiteral("on") : QStringLiteral("off")));
        }

        if (beforeValues.style.fill_color != afterValues.style.fill_color)
        {
            changes.append(
                QStringLiteral("fill color %1 -> %2")
                    .arg(formatColorText(beforeValues.style.fill_color), formatColorText(afterValues.style.fill_color)));
        }
    }

    if (kind != PrimitiveKind::Point && beforeValues.style.width != afterValues.style.width)
    {
        changes.append(
            QStringLiteral("width %1 -> %2")
                .arg(formatNumber(beforeValues.style.width), formatNumber(afterValues.style.width)));
    }

    if (beforeValues.style.point_size != afterValues.style.point_size)
    {
        changes.append(
            QStringLiteral("point size %1 -> %2")
                .arg(formatNumber(beforeValues.style.point_size), formatNumber(afterValues.style.point_size)));
    }

    if (!pointsMatch(beforeValues.points, afterValues.points))
    {
        changes.append(
            QStringLiteral("coordinates %1 -> %2 vertices")
                .arg(beforeValues.points.size())
                .arg(afterValues.points.size()));
    }

    return changes.isEmpty() ? QStringLiteral("no visible value changes") : changes.join(QStringLiteral(", "));
}

/// Formats validation errors for the log panel.
QString validationLogSummary(const PrimitiveEditValidationErrors &errors)
{
    QStringList issues;

    if (!errors.stroke_color.isEmpty())
    {
        issues.append(QStringLiteral("stroke color: %1").arg(errors.stroke_color));
    }

    if (!errors.fill_color.isEmpty())
    {
        issues.append(QStringLiteral("fill color: %1").arg(errors.fill_color));
    }

    if (!errors.width.isEmpty())
    {
        issues.append(QStringLiteral("width: %1").arg(errors.width));
    }

    if (!errors.point_size.isEmpty())
    {
        issues.append(QStringLiteral("point size: %1").arg(errors.point_size));
    }

    if (!errors.coordinates.isEmpty())
    {
        issues.append(QStringLiteral("coordinates: %1").arg(errors.coordinates));
    }

    return issues.join(QStringLiteral("; "));
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenuBar();
    setupViewportControls();
    setupStatusBar();
    updateUiFromScene();
}

void MainWindow::openPlyFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open PolyShow .ply File"),
        QString(),
        plyFileDialogFilter());

    if (filePath.isEmpty())
    {
        return;
    }

    importFiles(QStringList {filePath}, true);
}

void MainWindow::importPlyFiles()
{
    const QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("Import PolyShow .ply Files"),
        QString(),
        plyFileDialogFilter());

    if (filePaths.isEmpty())
    {
        return;
    }

    importFiles(filePaths, false);
}

void MainWindow::setRenderModeSolid()
{
    setRenderMode(GeometryScene::RenderMode::Solid);
}

void MainWindow::setRenderModeWireframe()
{
    setRenderMode(GeometryScene::RenderMode::Wireframe);
}

void MainWindow::setRenderModePoints()
{
    setRenderMode(GeometryScene::RenderMode::Points);
}

void MainWindow::updateMousePosition(const QPointF &scenePosition)
{
    m_status_mouse_label->setText(
        QStringLiteral("X: %1  Y: %2").arg(scenePosition.x(), 0, 'f', 2).arg(scenePosition.y(), 0, 'f', 2));
}

void MainWindow::onGeometryChanged(int pointCount, int polylineCount, int polygonCount)
{
    m_status_info_label->setText(
        QStringLiteral("Points: %1  Polylines: %2  Polygons: %3").arg(pointCount).arg(polylineCount).arg(polygonCount));
}

void MainWindow::onLayerVisibilityChanged(int layerIndex, bool visible)
{
    if (layerIndex < 0 || layerIndex >= m_document_data.layers.size())
    {
        return;
    }

    m_document_data.layers[layerIndex].visible = visible;
    m_layer_sidebar->setDocumentData(m_document_data, false);
    m_scene->setDocumentData(m_document_data);
    m_inspector_panel->setDocumentData(m_document_data);
    onSelectionStateChanged(normalizedSelectionState(m_selection_state));
}

void MainWindow::onPrimitiveVisibilityChanged(int layerIndex, int primitiveIndex, bool visible)
{
    if (layerIndex < 0 || layerIndex >= m_document_data.layers.size())
    {
        return;
    }

    LayerData &layer = m_document_data.layers[layerIndex];
    if (primitiveIndex < 0 || primitiveIndex >= layer.primitives.size())
    {
        return;
    }

    layer.primitives[primitiveIndex].visible = visible;
    m_layer_sidebar->setDocumentData(m_document_data, false);
    m_scene->setDocumentData(m_document_data);
    m_inspector_panel->setDocumentData(m_document_data);
    onSelectionStateChanged(normalizedSelectionState(m_selection_state));
}

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

void MainWindow::onSelectionStateChanged(const SelectionState &selectionState)
{
    m_selection_state = normalizedSelectionState(selectionState);
    m_scene->setSelectionState(m_selection_state);
    m_layer_sidebar->setSelectionState(m_selection_state);
    m_inspector_panel->setSelectionState(m_selection_state);
    const bool showInspector = m_selection_state.kind != SelectionKind::None;
    m_inspector_container->setVisible(showInspector);
    if (showInspector)
    {
        QList<int> sizes = m_splitter->sizes();
        if (sizes.size() == 3 && sizes.at(2) == 0)
        {
            m_splitter->setSizes({300, 700, 332});
        }
    }
    m_status_info_label->setText(
        QStringLiteral("Points: %1  Polylines: %2  Polygons: %3")
            .arg(m_scene->pointCount())
            .arg(m_scene->polylineCount())
            .arg(m_scene->polygonCount()));
}

void MainWindow::onScenePrimitiveActivated(int layerIndex, int primitiveIndex)
{
    onSelectionStateChanged(SelectionState {SelectionKind::Primitive, layerIndex, primitiveIndex});
}

void MainWindow::onEmptySceneActivated()
{
    onSelectionStateChanged(SelectionState {});
}

void MainWindow::onInspectorApplyRequested(const PrimitiveEditRequest &request)
{
    if (request.layer_index < 0 || request.layer_index >= m_document_data.layers.size())
    {
        return;
    }

    const LayerData &currentLayer = m_document_data.layers.at(request.layer_index);
    if (request.primitive_index < 0 || request.primitive_index >= currentLayer.primitives.size())
    {
        return;
    }

    const PrimitiveKind expectedKind = currentLayer.primitives.at(request.primitive_index).reference.kind;
    const QString targetLabel = primitiveLogLabel(m_document_data, request.layer_index, request.primitive_index);
    m_log_panel->appendMessage(
        LogSeverity::Info,
        QStringLiteral("[info] Applying inspector edits to %1").arg(targetLabel));

    if (request.primitive_kind != expectedKind)
    {
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to apply inspector edits to %1: selection kind changed").arg(targetLabel));
        statusBar()->showMessage(QStringLiteral("Inspector apply failed"), 3000);
        return;
    }

    PrimitiveEditValues beforeValues;
    if (!readPrimitiveEditValues(currentLayer, request.primitive_index, &beforeValues))
    {
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to read %1 before applying edits").arg(targetLabel));
        statusBar()->showMessage(QStringLiteral("Inspector apply failed"), 3000);
        return;
    }

    PrimitiveEditValidationErrors errors;
    PrimitiveEditValues parsedValues;
    if (!validatePrimitiveEditRequest(currentLayer, request.primitive_index, request, &parsedValues, &errors))
    {
        m_inspector_panel->setValidationErrors(errors);
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to apply inspector edits to %1: %2")
                .arg(targetLabel, validationLogSummary(errors)));
        statusBar()->showMessage(QStringLiteral("Inspector apply failed"), 3000);
        return;
    }

    m_inspector_panel->clearValidationErrors();
    if (!applyPrimitiveEditValues(m_document_data.layers[request.layer_index], request.primitive_index, parsedValues))
    {
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to write inspector edits to %1").arg(targetLabel));
        statusBar()->showMessage(QStringLiteral("Inspector apply failed"), 3000);
        return;
    }

    m_log_panel->appendMessage(
        LogSeverity::Info,
        QStringLiteral("[info] Updated %1: %2")
            .arg(targetLabel, primitiveEditSummary(expectedKind, beforeValues, parsedValues)));
    syncDocumentToViews(false);
    statusBar()->showMessage(QStringLiteral("Primitive updated"), 3000);
}

void MainWindow::onInspectorResetRequested()
{
    if (m_selection_state.kind != SelectionKind::Primitive)
    {
        return;
    }

    const QString targetLabel = primitiveLogLabel(m_document_data, m_selection_state.layer_index, m_selection_state.primitive_index);
    m_log_panel->appendMessage(
        LogSeverity::Info,
        QStringLiteral("[info] Reset unsaved inspector edits for %1").arg(targetLabel));
    statusBar()->showMessage(QStringLiteral("Inspector edits reset"), 2000);
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(
        this,
        QStringLiteral("About PolyShow"),
        QStringLiteral("PolyShow MVP\n\n") + QStringLiteral("Features:\n")
            + QStringLiteral("1. Open or import PolyShow .ply files\n")
            + QStringLiteral("2. Display points, polylines, and polygons\n")
            + QStringLiteral("3. Toggle layer and primitive visibility\n")
            + QStringLiteral("4. Search and inspect imported geometry\n")
            + QStringLiteral("5. Switch render mode (Solid/Wireframe/Points)"));
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("PolyShow"));
    resize(1280, 800);
    const ThemeColors &themeColors = UiTheme::colors(m_theme_mode);

    m_scene = new GeometryScene(this);
    m_geometry_viewer = new GeometryViewer(this);
    m_geometry_viewer->setScene(m_scene);

    m_layer_sidebar = new LayerSidebar(this);
    m_inspector_panel = new InspectorPanel(this);
    m_log_panel = new LogPanel(this);

    auto *leftContainer = new PanelFrame(PanelFrame::Variant::Panel, this);
    auto *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(12, 12, 12, 12);
    leftLayout->addWidget(m_layer_sidebar);

    m_viewport_frame = new PanelFrame(PanelFrame::Variant::Canvas, this);
    auto *viewportLayout = new QVBoxLayout(m_viewport_frame);
    viewportLayout->setContentsMargins(12, 12, 12, 12);
    viewportLayout->setSpacing(10);
    m_viewport_controls_widget = new QWidget(m_viewport_frame);
    viewportLayout->addWidget(m_viewport_controls_widget);
    viewportLayout->addWidget(m_geometry_viewer, 1);

    m_inspector_container = new PanelFrame(PanelFrame::Variant::Panel, this);
    auto *inspectorLayout = new QVBoxLayout(m_inspector_container);
    inspectorLayout->setContentsMargins(12, 12, 12, 12);
    inspectorLayout->addWidget(m_inspector_panel);
    m_inspector_container->setVisible(false);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(8);
    m_splitter->addWidget(leftContainer);
    m_splitter->addWidget(m_viewport_frame);
    m_splitter->addWidget(m_inspector_container);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
    m_splitter->setSizes({300, 700, 0});

    m_log_panel_container = new PanelFrame(PanelFrame::Variant::Panel, this);
    auto *logLayout = new QVBoxLayout(m_log_panel_container);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->addWidget(m_log_panel);
    m_log_panel_container->setMinimumHeight(96);

    m_vertical_splitter = new QSplitter(Qt::Vertical, this);
    m_vertical_splitter->setChildrenCollapsible(false);
    m_vertical_splitter->setHandleWidth(8);
    m_vertical_splitter->addWidget(m_splitter);
    m_vertical_splitter->addWidget(m_log_panel_container);
    m_vertical_splitter->setStretchFactor(0, 1);
    m_vertical_splitter->setStretchFactor(1, 0);
    m_vertical_splitter->setSizes({640, 140});

    auto *centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("workspaceRoot"));
    centralWidget->setAutoFillBackground(true);
    centralWidget->setAttribute(Qt::WA_StyledBackground, true);
    // centralWidget->setStyleSheet(QStringLiteral("#workspaceRoot { background-color: %1; }")
    //                                  .arg(themeColors.window_background.name(QColor::HexArgb)));
    QPalette centralPalette = centralWidget->palette();
    centralPalette.setColor(QPalette::Window, themeColors.window_background);
    centralWidget->setPalette(centralPalette);
    auto *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);
    layout->addWidget(m_vertical_splitter, 1);
    setCentralWidget(centralWidget);

    const QString splitterStyle = QStringLiteral(
        "QSplitter::handle {"
        "  background-color: %1;"
        "}"
        "QSplitter::handle:hover {"
        "  background-color: %2;"
        "}"
        "QSplitter::handle:pressed {"
        "  background-color: %2;"
        "}")
                                      .arg(themeColors.border_subtle.name(QColor::HexArgb))
                                      .arg(themeColors.text_muted.name(QColor::HexArgb));
    // m_splitter->setStyleSheet(splitterStyle);
    // m_vertical_splitter->setStyleSheet(splitterStyle);

    connect(m_scene, &GeometryScene::geometryChanged, this, &MainWindow::onGeometryChanged);
    connect(m_geometry_viewer, &GeometryViewer::mousePositionChanged, this, &MainWindow::updateMousePosition);
    connect(m_geometry_viewer, &GeometryViewer::primitiveActivated, this, &MainWindow::onScenePrimitiveActivated);
    connect(m_geometry_viewer, &GeometryViewer::emptyAreaActivated, this, &MainWindow::onEmptySceneActivated);
    connect(m_layer_sidebar, &LayerSidebar::selectionChanged, this, &MainWindow::onSelectionStateChanged);
    connect(m_layer_sidebar, &LayerSidebar::layerVisibilityChanged, this, &MainWindow::onLayerVisibilityChanged);
    connect(m_layer_sidebar, &LayerSidebar::primitiveVisibilityChanged, this, &MainWindow::onPrimitiveVisibilityChanged);
    connect(m_inspector_panel, &InspectorPanel::applyRequested, this, &MainWindow::onInspectorApplyRequested);
    connect(m_inspector_panel, &InspectorPanel::resetRequested, this, &MainWindow::onInspectorResetRequested);
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu(QStringLiteral("File"));

    m_open_action = new QAction(QStringLiteral("Open .ply..."), this);
    m_open_action->setShortcut(QKeySequence::Open);
    connect(m_open_action, &QAction::triggered, this, &MainWindow::openPlyFile);
    fileMenu->addAction(m_open_action);

    m_import_action = new QAction(QStringLiteral("Import .ply..."), this);
    connect(m_import_action, &QAction::triggered, this, &MainWindow::importPlyFiles);
    fileMenu->addAction(m_import_action);

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

void MainWindow::setupStatusBar()
{
    m_status_info_label = new QLabel(QStringLiteral("Points: 0  Polylines: 0  Polygons: 0"), this);
    m_status_mouse_label = new QLabel(QStringLiteral("X: 0.00  Y: 0.00"), this);

    statusBar()->addPermanentWidget(m_status_info_label);
    statusBar()->addPermanentWidget(m_status_mouse_label);
    statusBar()->showMessage(QStringLiteral("Ready"), 1500);
}

void MainWindow::setupViewportControls()
{
    auto *layout = new QHBoxLayout(m_viewport_controls_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *panButton = new PillButton(QStringLiteral("Pan"), m_viewport_controls_widget);
    panButton->setVariant(PillButton::Variant::Primary);
    panButton->setCheckable(true);
    panButton->setChecked(true);
    layout->addWidget(panButton);

    auto *fitButton = new PillButton(QStringLiteral("Fit View"), m_viewport_controls_widget);
    layout->addWidget(fitButton);
    connect(fitButton, &QPushButton::clicked, m_fit_action, &QAction::trigger);

    auto *zoomOutButton = new PillButton(QStringLiteral("Zoom -"), m_viewport_controls_widget);
    layout->addWidget(zoomOutButton);
    connect(zoomOutButton, &QPushButton::clicked, m_zoom_out_action, &QAction::trigger);

    auto *zoomInButton = new PillButton(QStringLiteral("Zoom +"), m_viewport_controls_widget);
    layout->addWidget(zoomInButton);
    connect(zoomInButton, &QPushButton::clicked, m_zoom_in_action, &QAction::trigger);

    auto *resetButton = new PillButton(QStringLiteral("Reset"), m_viewport_controls_widget);
    layout->addWidget(resetButton);
    connect(resetButton, &QPushButton::clicked, m_reset_view_action, &QAction::trigger);

    layout->addStretch();

    m_grid_toggle_button = new PillButton(QStringLiteral("Grid On"), m_viewport_controls_widget);
    m_grid_toggle_button->setCheckable(true);
    layout->addWidget(m_grid_toggle_button);
    connect(m_grid_toggle_button, &QPushButton::clicked, this, [this](bool checked) {
        m_scene->setGridVisible(checked);
        updateViewportControlState();
    });

    m_render_mode_combo_box = new QComboBox(m_viewport_controls_widget);
    m_render_mode_combo_box->addItem(QStringLiteral("Solid"));
    m_render_mode_combo_box->addItem(QStringLiteral("Wireframe"));
    m_render_mode_combo_box->addItem(QStringLiteral("Points"));
    connect(
        m_render_mode_combo_box,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &MainWindow::onRenderModeChanged);
    layout->addWidget(m_render_mode_combo_box);

    updateViewportControlState();
}

void MainWindow::updateViewportControlState()
{
    if (m_grid_toggle_button != nullptr)
    {
        const QSignalBlocker blocker(m_grid_toggle_button);
        m_grid_toggle_button->setChecked(m_scene->isGridVisible());
        m_grid_toggle_button->setText(m_scene->isGridVisible() ? QStringLiteral("Grid On") : QStringLiteral("Grid Off"));
        m_grid_toggle_button->setVariant(
            m_scene->isGridVisible() ? PillButton::Variant::Success : PillButton::Variant::Neutral);
    }

    if (m_render_mode_combo_box != nullptr)
    {
        int comboIndex = 0;
        switch (m_scene->renderMode())
        {
        case GeometryScene::RenderMode::Solid:
            comboIndex = 0;
            break;
        case GeometryScene::RenderMode::Wireframe:
            comboIndex = 1;
            break;
        case GeometryScene::RenderMode::Points:
            comboIndex = 2;
            break;
        default:
            break;
        }

        const QSignalBlocker blocker(m_render_mode_combo_box);
        m_render_mode_combo_box->setCurrentIndex(comboIndex);
    }
}

void MainWindow::setRenderMode(GeometryScene::RenderMode renderMode)
{
    m_scene->setRenderMode(renderMode);

    QAction *checkedAction = m_solid_mode_action;
    switch (renderMode)
    {
    case GeometryScene::RenderMode::Solid:
        checkedAction = m_solid_mode_action;
        break;
    case GeometryScene::RenderMode::Wireframe:
        checkedAction = m_wireframe_mode_action;
        break;
    case GeometryScene::RenderMode::Points:
        checkedAction = m_points_mode_action;
        break;
    default:
        break;
    }

    checkedAction->setChecked(true);
    updateViewportControlState();
}

void MainWindow::importFiles(const QStringList &filePaths, bool replaceExisting)
{
    DocumentData nextDocument = replaceExisting ? DocumentData {} : m_document_data;
    QStringList failureMessages;
    int importedCount = 0;

    for (const QString &filePath : filePaths)
    {
        GeometryData geometryData;
        QString errorMessage;
        if (!PlyParser::parseFile(filePath, geometryData, &errorMessage))
        {
            failureMessages.append(QStringLiteral("%1\n%2").arg(filePath, errorMessage));
            m_log_panel->appendMessage(
                LogSeverity::Error,
                QStringLiteral("[error] %1").arg(QStringLiteral("%1: %2").arg(filePath, errorMessage)));
            continue;
        }

        nextDocument.layers.append(buildLayerData(filePath, geometryData));
        ++importedCount;
        m_log_panel->appendMessage(
            LogSeverity::Info,
            QStringLiteral("[info] %1 imported successfully").arg(QFileInfo(filePath).fileName()));
    }

    if (importedCount == 0)
    {
        const QString message = failureMessages.isEmpty()
            ? QStringLiteral("No files were imported.")
            : failureMessages.join(QStringLiteral("\n\n"));
        QMessageBox::critical(this, QStringLiteral("Import Failed"), message);
        statusBar()->showMessage(QStringLiteral("Import failed"), 3000);
        return;
    }

    m_document_data = nextDocument;
    m_selection_state = {};
    syncDocumentToViews(true);

    if (failureMessages.isEmpty())
    {
        const QString successMessage = replaceExisting
            ? QStringLiteral("File loaded")
            : QStringLiteral("Imported %1 layer(s)").arg(importedCount);
        statusBar()->showMessage(successMessage, 3000);
        return;
    }

    m_log_panel->appendMessage(
        LogSeverity::Warning,
        QStringLiteral("[warning] Imported %1 file(s), %2 failed").arg(importedCount).arg(failureMessages.size()));
    QMessageBox::warning(
        this,
        QStringLiteral("Import Completed with Errors"),
        QStringLiteral("Imported %1 file(s). %2 file(s) failed.\n\n%3")
            .arg(importedCount)
            .arg(failureMessages.size())
            .arg(failureMessages.join(QStringLiteral("\n\n"))));
    statusBar()->showMessage(
        QStringLiteral("Imported %1 file(s), %2 failed").arg(importedCount).arg(failureMessages.size()),
        4000);
}

void MainWindow::syncDocumentToViews(bool fitScene)
{
    m_selection_state = normalizedSelectionState(m_selection_state);
    m_layer_sidebar->setDocumentData(m_document_data, true);
    m_scene->setDocumentData(m_document_data);
    m_inspector_panel->setDocumentData(m_document_data);
    onSelectionStateChanged(m_selection_state);

    if (fitScene)
    {
        m_geometry_viewer->fitScene();
    }
}

SelectionState MainWindow::normalizedSelectionState(const SelectionState &selectionState) const
{
    return normalizedSelection(m_document_data, selectionState);
}

void MainWindow::updateUiFromScene()
{
    setRenderMode(m_scene->renderMode());
    onGeometryChanged(m_scene->pointCount(), m_scene->polylineCount(), m_scene->polygonCount());
    updateViewportControlState();
    onSelectionStateChanged(m_selection_state);
}

} // namespace PolyShow
