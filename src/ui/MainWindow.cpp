#include "ui/MainWindow.h"

#include "core/LayerEditing.h"
#include "parsers/PlyParser.h"
#include "parsers/PlySerializer.h"
#include "ui/IconButton.h"
#include "ui/InspectorPanel.h"
#include "ui/LayerSidebar.h"
#include "ui/LogPanel.h"
#include "ui/MaterialIcon.h"
#include "ui/PanelFrame.h"
#include "ui/PillButton.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStatusBar>
#include <QThread>
#include <QToolButton>
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

QString ipcNativeEndpoint()
{
#ifdef Q_OS_WIN
    return QStringLiteral(R"(\\.\pipe\polyshow-ipc)");
#else
    return QStringLiteral("/tmp/polyshow-ipc.sock");
#endif
}

QIcon materialIcon(const QString &iconName)
{
    return MaterialIcon::icon(iconName);
}

QString normalizedLayerBaseName(const QString &name)
{
    const QString trimmedName = name.trimmed();
    return trimmedName.isEmpty() ? QStringLiteral("Layer") : trimmedName;
}

QString uniqueLayerNameForDocument(const DocumentData &documentData, const QString &baseName)
{
    const QString normalizedBaseName = normalizedLayerBaseName(baseName);
    if (findLayerByName(documentData, normalizedBaseName) == nullptr)
    {
        return normalizedBaseName;
    }

    for (int suffix = 2;; ++suffix)
    {
        const QString candidate = QStringLiteral("%1 (%2)").arg(normalizedBaseName).arg(suffix);
        if (findLayerByName(documentData, candidate) == nullptr)
        {
            return candidate;
        }
    }
}

QString layerTypeLogText(LayerType layerType)
{
    switch (layerType)
    {
    case LayerType::ExternalFileNormal:
        return QStringLiteral("file layer");
    case LayerType::InternalNormal:
        return QStringLiteral("internal layer");
    case LayerType::InternalIpc:
        return QStringLiteral("IPC layer");
    default:
        return QStringLiteral("layer");
    }
}

/// Returns the display label for one runtime primitive category.
QString primitiveCategoryLabel(PrimitiveKind kind, int vertexCount)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        return QStringLiteral("Point");
    case PrimitiveKind::Polyline:
        return vertexCount == 2 ? QStringLiteral("Line") : QStringLiteral("Polyline");
    case PrimitiveKind::Polygon:
        return QStringLiteral("Polygon");
    default:
        return QStringLiteral("Primitive");
    }
}

/// Returns the vertex count for one primitive reference.
int primitiveVertexCount(const GeometryData &geometryData, PrimitiveKind kind, int index)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        return 1;
    case PrimitiveKind::Polyline:
        return geometryData.polylines.value(index).vertices.size();
    case PrimitiveKind::Polygon:
        return geometryData.polygons.value(index).vertices.size();
    default:
        return 0;
    }
}

/// Returns the optional custom name stored on one parsed primitive.
QString primitiveCustomName(const GeometryData &geometryData, PrimitiveKind kind, int index)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        return geometryData.points.value(index).name;
    case PrimitiveKind::Polyline:
        return geometryData.polylines.value(index).name;
    case PrimitiveKind::Polygon:
        return geometryData.polygons.value(index).name;
    default:
        return {};
    }
}

bool layerHasVisiblePrimitives(const LayerData &layer)
{
    if (layer.primitives.isEmpty())
    {
        return true;
    }

    for (const LayerPrimitiveData &primitive : layer.primitives)
    {
        if (primitive.visible)
        {
            return true;
        }
    }

    return false;
}

void syncLayerVisibilitySummary(LayerData *layer)
{
    if (layer == nullptr)
    {
        return;
    }

    layer->visible = layerHasVisiblePrimitives(*layer);
}

/// Returns the default display label for one primitive without a custom name.
QString nextPrimitiveDisplayName(
    const GeometryData &geometryData,
    PrimitiveKind kind,
    int index,
    int &pointOrdinal,
    int &lineOrdinal,
    int &polylineOrdinal,
    int &polygonOrdinal)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        ++pointOrdinal;
        return QStringLiteral("%1 %2").arg(primitiveCategoryLabel(kind, 1)).arg(pointOrdinal);
    case PrimitiveKind::Polyline:
    {
        const int vertexCount = primitiveVertexCount(geometryData, kind, index);
        if (vertexCount == 2)
        {
            ++lineOrdinal;
            return QStringLiteral("%1 %2").arg(primitiveCategoryLabel(kind, vertexCount)).arg(lineOrdinal);
        }
        ++polylineOrdinal;
        return QStringLiteral("%1 %2").arg(primitiveCategoryLabel(kind, vertexCount)).arg(polylineOrdinal);
    }
    case PrimitiveKind::Polygon:
        ++polygonOrdinal;
        return QStringLiteral("%1 %2").arg(primitiveCategoryLabel(kind, primitiveVertexCount(geometryData, kind, index)))
            .arg(polygonOrdinal);
    default:
        return QStringLiteral("Primitive");
    }
}

/// Appends one primitive entry to a layer list.
void appendPrimitiveEntry(
    QVector<LayerPrimitiveData> &primitives,
    const GeometryData &geometryData,
    PrimitiveKind kind,
    int index,
    int &pointOrdinal,
    int &lineOrdinal,
    int &polylineOrdinal,
    int &polygonOrdinal)
{
    const QString customName = primitiveCustomName(geometryData, kind, index);
    const QString displayName = customName.isEmpty()
        ? nextPrimitiveDisplayName(geometryData, kind, index, pointOrdinal, lineOrdinal, polylineOrdinal, polygonOrdinal)
        : customName;
    primitives.append(LayerPrimitiveData {PrimitiveReference {kind, index}, displayName, true});
}

bool promptForNewLayer(
    QWidget *parent, const DocumentData &documentData, const QString &defaultLayerName, QString *layerName, LayerType *layerType)
{
    if (layerName == nullptr || layerType == nullptr)
    {
        return false;
    }

    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("New Layer"));

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto *formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(8);

    auto *nameEdit = new QLineEdit(defaultLayerName, &dialog);
    formLayout->addRow(QStringLiteral("Layer Name"), nameEdit);

    auto *typeComboBox = new QComboBox(&dialog);
    typeComboBox->addItem(QStringLiteral("Internal Normal"), static_cast<int>(LayerType::InternalNormal));
    typeComboBox->addItem(QStringLiteral("IPC Layer"), static_cast<int>(LayerType::InternalIpc));
    formLayout->addRow(QStringLiteral("Layer Type"), typeComboBox);

    layout->addLayout(formLayout);

    auto *errorLabel = new QLabel(&dialog);
    errorLabel->setProperty("role", QStringLiteral("validationError"));
    errorLabel->setWordWrap(true);
    errorLabel->setVisible(false);
    layout->addWidget(errorLabel);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttonBox);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *okButton = buttonBox->button(QDialogButtonBox::Ok);
    const auto validate = [&documentData, errorLabel, nameEdit, okButton]() {
        const QString candidateName = nameEdit->text().trimmed();

        QString errorMessage;
        if (candidateName.isEmpty())
        {
            errorMessage = QStringLiteral("Layer name is required.");
        }
        else if (findLayerByName(documentData, candidateName) != nullptr)
        {
            errorMessage = QStringLiteral("Layer name must be unique within the document.");
        }

        errorLabel->setText(errorMessage);
        errorLabel->setVisible(!errorMessage.isEmpty());
        if (okButton != nullptr)
        {
            okButton->setEnabled(errorMessage.isEmpty());
        }
    };

    QObject::connect(nameEdit, &QLineEdit::textChanged, &dialog, validate);
    validate();
    nameEdit->selectAll();
    nameEdit->setFocus(Qt::OtherFocusReason);

    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    *layerName = nameEdit->text().trimmed();
    *layerType = static_cast<LayerType>(typeComboBox->currentData().toInt());
    return true;
}

/// Builds the runtime layer state for one successfully parsed file.
LayerData buildLayerData(const QString &filePath, const QString &layerName, const GeometryData &geometryData)
{
    LayerData layer = createEmptyLayer(layerName, LayerType::ExternalFileNormal, filePath);
    layer.geometry = geometryData;

    int pointOrdinal = 0;
    int lineOrdinal = 0;
    int polylineOrdinal = 0;
    int polygonOrdinal = 0;

    if (!geometryData.primitive_order.isEmpty())
    {
        for (const PrimitiveReference &reference : geometryData.primitive_order)
        {
            switch (reference.kind)
            {
            case PrimitiveKind::Point:
                appendPrimitiveEntry(
                    layer.primitives,
                    geometryData,
                    reference.kind,
                    reference.index,
                    pointOrdinal,
                    lineOrdinal,
                    polylineOrdinal,
                    polygonOrdinal);
                break;
            case PrimitiveKind::Polyline:
                appendPrimitiveEntry(
                    layer.primitives,
                    geometryData,
                    reference.kind,
                    reference.index,
                    pointOrdinal,
                    lineOrdinal,
                    polylineOrdinal,
                    polygonOrdinal);
                break;
            case PrimitiveKind::Polygon:
                appendPrimitiveEntry(
                    layer.primitives,
                    geometryData,
                    reference.kind,
                    reference.index,
                    pointOrdinal,
                    lineOrdinal,
                    polylineOrdinal,
                    polygonOrdinal);
                break;
            }
        }

        syncLayerVisibilitySummary(&layer);
        return layer;
    }

    for (int index = 0; index < geometryData.points.size(); ++index)
    {
        appendPrimitiveEntry(
            layer.primitives,
            geometryData,
            PrimitiveKind::Point,
            index,
            pointOrdinal,
            lineOrdinal,
            polylineOrdinal,
            polygonOrdinal);
    }

    for (int index = 0; index < geometryData.polylines.size(); ++index)
    {
        appendPrimitiveEntry(
            layer.primitives,
            geometryData,
            PrimitiveKind::Polyline,
            index,
            pointOrdinal,
            lineOrdinal,
            polylineOrdinal,
            polygonOrdinal);
    }

    for (int index = 0; index < geometryData.polygons.size(); ++index)
    {
        appendPrimitiveEntry(
            layer.primitives,
            geometryData,
            PrimitiveKind::Polygon,
            index,
            pointOrdinal,
            lineOrdinal,
            polylineOrdinal,
            polygonOrdinal);
    }

    syncLayerVisibilitySummary(&layer);
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
        return selectionState;
    }

    const LayerData &layer = documentData.layers.at(selectionState.layer_index);
    if (selectionState.primitive_index < 0 || selectionState.primitive_index >= layer.primitives.size())
    {
        return {};
    }

    // Visibility is an editable property of the selection target, so hidden primitives stay selectable in the outliner.
    return selectionState;
}

/// Returns whether the selection points at one primitive.
bool isPrimitiveSelection(const SelectionState &selectionState)
{
    return selectionState.kind == SelectionKind::Primitive;
}

/// Returns whether the primitive selection already exists in one selection list.
bool containsPrimitiveSelection(const QVector<SelectionState> &selectionStates, const SelectionState &selectionState)
{
    return std::any_of(
        selectionStates.cbegin(),
        selectionStates.cend(),
        [&selectionState](const SelectionState &candidate) {
            return candidate == selectionState;
        });
}

/// Builds a selection set from one primary selection request.
SelectionSet selectionSetFromPrimarySelection(const SelectionState &selectionState)
{
    SelectionSet selectionSet;
    selectionSet.primary_selection = selectionState;
    if (isPrimitiveSelection(selectionState))
    {
        selectionSet.selected_primitives.append(selectionState);
    }
    return selectionSet;
}

/// Returns one selection set normalized against the current document shape.
SelectionSet normalizedSelectionSetForDocument(const DocumentData &documentData, const SelectionSet &selectionSet)
{
    const SelectionState primarySelection = normalizedSelection(documentData, selectionSet.primary_selection);

    // Layer rows remain single-selection anchors and should never carry primitive multi-selection state.
    if (primarySelection.kind == SelectionKind::Layer)
    {
        SelectionSet normalizedSelectionSet;
        normalizedSelectionSet.primary_selection = primarySelection;
        return normalizedSelectionSet;
    }

    SelectionSet normalizedSelectionSet;
    for (const SelectionState &selectionState : selectionSet.selected_primitives)
    {
        const SelectionState normalizedCandidate = normalizedSelection(documentData, selectionState);
        if (isPrimitiveSelection(normalizedCandidate)
            && !containsPrimitiveSelection(normalizedSelectionSet.selected_primitives, normalizedCandidate))
        {
            normalizedSelectionSet.selected_primitives.append(normalizedCandidate);
        }
    }

    if (isPrimitiveSelection(primarySelection))
    {
        if (!containsPrimitiveSelection(normalizedSelectionSet.selected_primitives, primarySelection))
        {
            normalizedSelectionSet.selected_primitives.append(primarySelection);
        }
        normalizedSelectionSet.primary_selection = primarySelection;
        return normalizedSelectionSet;
    }

    if (!normalizedSelectionSet.selected_primitives.isEmpty())
    {
        normalizedSelectionSet.primary_selection = normalizedSelectionSet.selected_primitives.last();
    }

    return normalizedSelectionSet;
}

/// Toggles one primitive inside a normalized primitive selection set.
SelectionSet toggledPrimitiveSelectionSet(const SelectionSet &currentSelectionSet, const SelectionState &selectionState)
{
    SelectionSet nextSelectionSet = currentSelectionSet;
    if (nextSelectionSet.primary_selection.kind == SelectionKind::Layer)
    {
        nextSelectionSet = {};
    }

    for (int index = 0; index < nextSelectionSet.selected_primitives.size(); ++index)
    {
        if (nextSelectionSet.selected_primitives.at(index) != selectionState)
        {
            continue;
        }

        nextSelectionSet.selected_primitives.removeAt(index);
        if (nextSelectionSet.selected_primitives.isEmpty())
        {
            return {};
        }

        if (nextSelectionSet.primary_selection == selectionState
            || !containsPrimitiveSelection(nextSelectionSet.selected_primitives, nextSelectionSet.primary_selection))
        {
            nextSelectionSet.primary_selection = nextSelectionSet.selected_primitives.last();
        }
        return nextSelectionSet;
    }

    nextSelectionSet.selected_primitives.append(selectionState);
    nextSelectionSet.primary_selection = selectionState;
    return nextSelectionSet;
}

/// Formats one numeric value for compact log output.
QString formatNumber(double value)
{
    return QString::number(value, 'f', 2);
}

/// Builds one primitive selection state.
SelectionState primitiveSelectionState(int layerIndex, int primitiveIndex)
{
    return SelectionState {SelectionKind::Primitive, layerIndex, primitiveIndex};
}

/// Returns whether the selection references one specific primitive.
bool matchesPrimitiveSelection(const SelectionState &selectionState, int layerIndex, int primitiveIndex)
{
    return selectionState.kind == SelectionKind::Primitive && selectionState.layer_index == layerIndex
        && selectionState.primitive_index == primitiveIndex;
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
QString styleFieldChangeSummary(
    PrimitiveStyleField field, const PrimitiveEditValues &beforeValues, const PrimitiveEditValues &afterValues)
{
    switch (field)
    {
    case PrimitiveStyleField::StrokeColor:
        return QStringLiteral("%1 -> %2")
            .arg(formatColorText(beforeValues.style.color), formatColorText(afterValues.style.color));
    case PrimitiveStyleField::FillColor:
        return QStringLiteral("%1 -> %2")
            .arg(formatColorText(beforeValues.style.fill_color), formatColorText(afterValues.style.fill_color));
    case PrimitiveStyleField::Width:
        return QStringLiteral("%1 -> %2")
            .arg(formatNumber(beforeValues.style.width), formatNumber(afterValues.style.width));
    case PrimitiveStyleField::PointSize:
        return QStringLiteral("%1 -> %2")
            .arg(formatNumber(beforeValues.style.point_size), formatNumber(afterValues.style.point_size));
    case PrimitiveStyleField::FillEnabled:
        return QStringLiteral("%1 -> %2")
            .arg(beforeValues.style.fill_enabled ? QStringLiteral("on") : QStringLiteral("off"))
            .arg(afterValues.style.fill_enabled ? QStringLiteral("on") : QStringLiteral("off"));
    default:
        return QStringLiteral("updated");
    }
}

/// Returns whether the requested style field actually changed.
bool styleFieldChanged(PrimitiveStyleField field, const PrimitiveEditValues &beforeValues, const PrimitiveEditValues &afterValues)
{
    switch (field)
    {
    case PrimitiveStyleField::StrokeColor:
        return beforeValues.style.color != afterValues.style.color;
    case PrimitiveStyleField::FillColor:
        return beforeValues.style.fill_color != afterValues.style.fill_color;
    case PrimitiveStyleField::Width:
        return beforeValues.style.width != afterValues.style.width;
    case PrimitiveStyleField::PointSize:
        return beforeValues.style.point_size != afterValues.style.point_size;
    case PrimitiveStyleField::FillEnabled:
        return beforeValues.style.fill_enabled != afterValues.style.fill_enabled;
    default:
        return false;
    }
}

SelectionState layerSelectionState(int layerIndex)
{
    return SelectionState {SelectionKind::Layer, layerIndex, -1};
}

QString layerLogLabel(const DocumentData &documentData, int layerIndex)
{
    if (layerIndex < 0 || layerIndex >= documentData.layers.size())
    {
        return QStringLiteral("active layer");
    }

    const QString displayName = documentData.layers.at(layerIndex).display_name;
    return displayName.isEmpty() ? QStringLiteral("Unnamed layer") : displayName;
}

PrimitiveKind primitiveKindForToolMode(GeometryViewer::ToolMode toolMode)
{
    switch (toolMode)
    {
    case GeometryViewer::ToolMode::DrawPoint:
        return PrimitiveKind::Point;
    case GeometryViewer::ToolMode::DrawPolyline:
        return PrimitiveKind::Polyline;
    case GeometryViewer::ToolMode::DrawPolygon:
        return PrimitiveKind::Polygon;
    default:
        return PrimitiveKind::Point;
    }
}

QString toolModeLabel(GeometryViewer::ToolMode toolMode)
{
    switch (toolMode)
    {
    case GeometryViewer::ToolMode::Browse:
        return QStringLiteral("Browse");
    case GeometryViewer::ToolMode::DrawPoint:
        return QStringLiteral("Point");
    case GeometryViewer::ToolMode::DrawPolyline:
        return QStringLiteral("Polyline");
    case GeometryViewer::ToolMode::DrawPolygon:
        return QStringLiteral("Polygon");
    default:
        return QStringLiteral("Browse");
    }
}

PrimitiveStyle drawingStyleForLayer(const LayerData &layer, PrimitiveKind kind)
{
    for (int primitiveIndex = layer.primitives.size() - 1; primitiveIndex >= 0; --primitiveIndex)
    {
        if (layer.primitives.at(primitiveIndex).reference.kind == kind)
        {
            return primitiveStyle(layer, primitiveIndex);
        }
    }

    return {};
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    qRegisterMetaType<IpcPrimitiveWriteMessage>("PolyShow::IpcPrimitiveWriteMessage");
    qRegisterMetaType<IpcPrimitiveWriteResult>("PolyShow::IpcPrimitiveWriteResult");

    setupUi();
    setupMenuBar();
    setupTopBar();
    setupViewportControls();
    setupStatusBar();
    updateUiFromScene();
}

MainWindow::~MainWindow()
{
    shutdownIpcListener(false);
}

void MainWindow::createLayer()
{
    QString layerName;
    LayerType layerType = LayerType::InternalNormal;
    if (!promptForNewLayer(this, m_document_data, nextLayerName(), &layerName, &layerType))
    {
        return;
    }

    m_document_data.layers.append(createEmptyLayer(layerName, layerType));
    m_active_layer_index = m_document_data.layers.size() - 1;
    m_selection_set = selectionSetFromPrimarySelection(layerSelectionState(m_active_layer_index));
    m_selection_state = m_selection_set.primary_selection;
    syncDocumentToViews(false);
    m_log_panel->appendMessage(
        LogSeverity::Info,
        QStringLiteral("[info] Created %1 %2").arg(layerTypeLogText(layerType), layerName));
    statusBar()->showMessage(QStringLiteral("Layer created"), 3000);
}

void MainWindow::exportActiveLayer()
{
    const int layerIndex = currentLayerIndex();
    if (layerIndex < 0 || layerIndex >= m_document_data.layers.size())
    {
        QMessageBox::information(this, QStringLiteral("Export Layer"), QStringLiteral("Select or create a layer first."));
        statusBar()->showMessage(QStringLiteral("No active layer"), 3000);
        return;
    }

    const LayerData &layer = m_document_data.layers.at(layerIndex);
    const QString defaultDirectory = layer.file_path.isEmpty()
        ? QDir::currentPath()
        : QFileInfo(layer.file_path).absolutePath();
    QString defaultFileName = layer.display_name.isEmpty() ? QStringLiteral("layer_%1").arg(layerIndex + 1) : layer.display_name;
    if (!defaultFileName.endsWith(QStringLiteral(".ply"), Qt::CaseInsensitive))
    {
        defaultFileName += QStringLiteral(".ply");
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Export Layer"),
        QDir(defaultDirectory).filePath(defaultFileName),
        plyFileDialogFilter());
    if (filePath.isEmpty())
    {
        return;
    }

    QString errorMessage;
    if (!PlySerializer::writeLayerFile(layer, filePath, &errorMessage))
    {
        QMessageBox::critical(this, QStringLiteral("Export Failed"), errorMessage);
        m_log_panel->appendMessage(LogSeverity::Error, QStringLiteral("[error] Failed to export %1: %2").arg(
                                                        layerLogLabel(m_document_data, layerIndex), errorMessage));
        statusBar()->showMessage(QStringLiteral("Layer export failed"), 3000);
        return;
    }

    m_document_data.layers[layerIndex].file_path = filePath;
    m_layer_sidebar->setDocumentData(m_document_data, true);
    m_layer_sidebar->setSelectionSet(m_selection_set);
    m_log_panel->appendMessage(
        LogSeverity::Info,
        QStringLiteral("[info] Exported %1 to %2").arg(layerLogLabel(m_document_data, layerIndex), filePath));
    statusBar()->showMessage(QStringLiteral("Layer exported"), 3000);
}

void MainWindow::openPlyFile()
{
    const QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("Open PolyShow .ply Files"),
        QString(),
        plyFileDialogFilter());

    if (filePaths.isEmpty())
    {
        return;
    }

    openFiles(filePaths);
}

void MainWindow::setToolModeBrowse()
{
    setWorkspaceToolMode(GeometryViewer::ToolMode::Browse);
}

void MainWindow::setToolModeDrawPoint()
{
    setWorkspaceToolMode(GeometryViewer::ToolMode::DrawPoint);
}

void MainWindow::setToolModeDrawPolyline()
{
    setWorkspaceToolMode(GeometryViewer::ToolMode::DrawPolyline);
}

void MainWindow::setToolModeDrawPolygon()
{
    setWorkspaceToolMode(GeometryViewer::ToolMode::DrawPolygon);
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

void MainWindow::onWorkspaceHoverChanged(const QPointF &scenePosition)
{
    if (m_workspace_tool_mode == GeometryViewer::ToolMode::Browse)
    {
        return;
    }

    m_has_drawing_hover_point = true;
    m_drawing_hover_point = Point2D {scenePosition.x(), scenePosition.y()};
    updateDrawingPreview();
}

void MainWindow::onWorkspaceHoverExited()
{
    if (!m_has_drawing_hover_point)
    {
        return;
    }

    m_has_drawing_hover_point = false;
    updateDrawingPreview();
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

    LayerData &layer = m_document_data.layers[layerIndex];
    for (LayerPrimitiveData &primitive : layer.primitives)
    {
        primitive.visible = visible;
    }
    syncLayerVisibilitySummary(&layer);
    refreshViewsForVisibilityChange();
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
    syncLayerVisibilitySummary(&layer);
    refreshViewsForVisibilityChange();
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

void MainWindow::onSelectionRequested(const SelectionState &selectionState, bool toggleRequested)
{
    const SelectionState normalizedSelectionStateValue = normalizedSelectionState(selectionState);
    if (normalizedSelectionStateValue.kind == SelectionKind::None)
    {
        if (!toggleRequested)
        {
            setSelectionSet({});
        }
        return;
    }

    if (normalizedSelectionStateValue.kind == SelectionKind::Layer)
    {
        setSelectionSet(selectionSetFromPrimarySelection(normalizedSelectionStateValue));
        return;
    }

    if (toggleRequested)
    {
        // Ctrl selection is primitive-only, so layer anchors are discarded before toggling primitives.
        setSelectionSet(toggledPrimitiveSelectionSet(m_selection_set, normalizedSelectionStateValue));
        return;
    }

    setSelectionSet(selectionSetFromPrimarySelection(normalizedSelectionStateValue));
}

void MainWindow::onScenePrimitiveActivated(int layerIndex, int primitiveIndex, bool toggleRequested)
{
    onSelectionRequested(SelectionState {SelectionKind::Primitive, layerIndex, primitiveIndex}, toggleRequested);
}

void MainWindow::onEmptySceneActivated(bool toggleRequested)
{
    if (!toggleRequested)
    {
        setSelectionSet({});
    }
}

void MainWindow::onInspectorStyleChangeRequested(const PrimitiveStyleChangeRequest &request)
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
    if (request.primitive_kind != expectedKind)
    {
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to update %1 %2: selection kind changed")
                .arg(targetLabel, primitiveStyleFieldText(request.field)));
        statusBar()->showMessage(QStringLiteral("Inspector update failed"), 3000);
        return;
    }

    PrimitiveEditValues beforeValues;
    if (!readPrimitiveEditValues(currentLayer, request.primitive_index, &beforeValues))
    {
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to read %1 before updating %2")
                .arg(targetLabel, primitiveStyleFieldText(request.field)));
        statusBar()->showMessage(QStringLiteral("Inspector update failed"), 3000);
        return;
    }

    PrimitiveEditValues parsedValues;
    QString errorMessage;
    if (!validateStyleChangeRequest(currentLayer, request.primitive_index, request, &parsedValues, &errorMessage))
    {
        m_inspector_panel->setStyleFieldError(request.field, errorMessage);
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to update %1 %2: %3")
                .arg(targetLabel, primitiveStyleFieldText(request.field), errorMessage));
        statusBar()->showMessage(QStringLiteral("Inspector update failed"), 3000);
        return;
    }

    m_inspector_panel->clearStyleFieldError(request.field);
    const bool didChange = styleFieldChanged(request.field, beforeValues, parsedValues);
    if (!didChange)
    {
        return;
    }

    if (!applyPrimitiveEditValues(m_document_data.layers[request.layer_index], request.primitive_index, parsedValues))
    {
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to write %1 %2")
                .arg(targetLabel, primitiveStyleFieldText(request.field)));
        statusBar()->showMessage(QStringLiteral("Inspector update failed"), 3000);
        return;
    }

    refreshSceneForPrimitiveEdit();
    m_log_panel->appendMessage(
        LogSeverity::Info,
        QStringLiteral("[info] Updated %1 %2: %3")
            .arg(targetLabel, primitiveStyleFieldText(request.field), styleFieldChangeSummary(request.field, beforeValues, parsedValues)));
    statusBar()->showMessage(QStringLiteral("Primitive updated"), 3000);
}

void MainWindow::onInspectorCoordinateDraftChanged(const PrimitiveCoordinateDraft &draft)
{
    if (draft.layer_index < 0 || draft.layer_index >= m_document_data.layers.size())
    {
        return;
    }

    const LayerData &currentLayer = m_document_data.layers.at(draft.layer_index);
    if (draft.primitive_index < 0 || draft.primitive_index >= currentLayer.primitives.size())
    {
        return;
    }

    const QString targetLabel = primitiveLogLabel(m_document_data, draft.layer_index, draft.primitive_index);
    QVector<Point2D> parsedPoints;
    QString errorMessage;
    if (!validateCoordinateDraft(currentLayer, draft.primitive_index, draft, &parsedPoints, &errorMessage))
    {
        const bool alreadyInvalid = m_edit_preview_state.hide_selected_primitive
            && matchesPrimitiveSelection(m_edit_preview_state.selection_state, draft.layer_index, draft.primitive_index);

        m_inspector_panel->setCoordinateError(errorMessage);
        m_edit_preview_state.selection_state = primitiveSelectionState(draft.layer_index, draft.primitive_index);
        m_edit_preview_state.hide_selected_primitive = true;
        m_scene->setEditPreviewState(m_edit_preview_state);

        if (!alreadyInvalid || !m_has_logged_invalid_coordinate_draft)
        {
            m_log_panel->appendMessage(
                LogSeverity::Error,
                QStringLiteral("[error] Invalid coordinates for %1: %2").arg(targetLabel, errorMessage));
            m_has_logged_invalid_coordinate_draft = true;
        }

        statusBar()->showMessage(QStringLiteral("Coordinate draft invalid"), 2000);
        return;
    }

    PrimitiveEditValues beforeValues;
    if (!readPrimitiveEditValues(currentLayer, draft.primitive_index, &beforeValues))
    {
        return;
    }

    const bool wasInvalid = m_edit_preview_state.hide_selected_primitive
        && matchesPrimitiveSelection(m_edit_preview_state.selection_state, draft.layer_index, draft.primitive_index);
    const bool pointsChanged = !pointsMatch(beforeValues.points, parsedPoints);

    clearCoordinatePreviewState();
    if (!pointsChanged)
    {
        if (wasInvalid)
        {
            m_scene->setEditPreviewState(m_edit_preview_state, true);
        }
        return;
    }

    PrimitiveEditValues nextValues = beforeValues;
    nextValues.points = parsedPoints;
    if (!applyPrimitiveEditValues(m_document_data.layers[draft.layer_index], draft.primitive_index, nextValues))
    {
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to write coordinates for %1").arg(targetLabel));
        statusBar()->showMessage(QStringLiteral("Coordinate update failed"), 3000);
        return;
    }

    refreshSceneForPrimitiveEdit();
    if (wasInvalid)
    {
        m_log_panel->appendMessage(
            LogSeverity::Info,
            QStringLiteral("[info] Restored coordinates for %1: %2 -> %3 vertices")
                .arg(targetLabel)
                .arg(beforeValues.points.size())
                .arg(nextValues.points.size()));
    }
}

void MainWindow::onDrawingPointRequested(const QPointF &scenePosition)
{
    const int layerIndex = ensureActiveLayer();
    if (layerIndex < 0 || layerIndex >= m_document_data.layers.size())
    {
        return;
    }

    if (m_workspace_tool_mode == GeometryViewer::ToolMode::DrawPoint)
    {
        const PrimitiveKind kind = primitiveKindForToolMode(m_workspace_tool_mode);
        const PrimitiveWriteRequest request {
            kind,
            QVector<Point2D> {Point2D {scenePosition.x(), scenePosition.y()}},
            drawingStyleForLayer(m_document_data.layers.at(layerIndex), kind)
        };

        int primitiveIndex = -1;
        QString errorMessage;
        if (!appendPrimitiveToLayer(m_document_data.layers[layerIndex], request, &primitiveIndex, &errorMessage))
        {
            m_log_panel->appendMessage(
                LogSeverity::Error,
                QStringLiteral("[error] Failed to add point to %1: %2")
                    .arg(layerLogLabel(m_document_data, layerIndex), errorMessage));
            statusBar()->showMessage(QStringLiteral("Point creation failed"), 3000);
            return;
        }

        m_selection_set = selectionSetFromPrimarySelection(primitiveSelectionState(layerIndex, primitiveIndex));
        m_selection_state = m_selection_set.primary_selection;
        m_has_drawing_hover_point = false;
        syncDocumentToViews(false);
        m_log_panel->appendMessage(
            LogSeverity::Info,
            QStringLiteral("[info] Added point to %1 at (%2, %3)")
                .arg(layerLogLabel(m_document_data, layerIndex))
                .arg(formatNumber(scenePosition.x()))
                .arg(formatNumber(scenePosition.y())));
        statusBar()->showMessage(QStringLiteral("Point added"), 2000);
        return;
    }

    m_drawing_points.append(Point2D {scenePosition.x(), scenePosition.y()});
    updateDrawingPreview();
    statusBar()->showMessage(
        QStringLiteral("%1 drawing: %2 vertex/vertices")
            .arg(toolModeLabel(m_workspace_tool_mode))
            .arg(m_drawing_points.size()),
        2000);
}

void MainWindow::finishDrawing()
{
    if (m_workspace_tool_mode == GeometryViewer::ToolMode::Browse
        || m_workspace_tool_mode == GeometryViewer::ToolMode::DrawPoint)
    {
        return;
    }

    if (m_drawing_points.isEmpty())
    {
        statusBar()->showMessage(QStringLiteral("Add vertices before finishing"), 2000);
        return;
    }

    const int layerIndex = ensureActiveLayer();
    if (layerIndex < 0 || layerIndex >= m_document_data.layers.size())
    {
        return;
    }

    const PrimitiveKind kind = primitiveKindForToolMode(m_workspace_tool_mode);
    const PrimitiveWriteRequest request {kind, m_drawing_points, drawingStyleForLayer(m_document_data.layers.at(layerIndex), kind)};

    int primitiveIndex = -1;
    QString errorMessage;
    if (!appendPrimitiveToLayer(m_document_data.layers[layerIndex], request, &primitiveIndex, &errorMessage))
    {
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to add %1 to %2: %3")
                .arg(toolModeLabel(m_workspace_tool_mode).toLower(), layerLogLabel(m_document_data, layerIndex), errorMessage));
        statusBar()->showMessage(QStringLiteral("Drawing commit failed"), 3000);
        return;
    }

    clearDrawingDraft(false);
    m_selection_set = selectionSetFromPrimarySelection(primitiveSelectionState(layerIndex, primitiveIndex));
    m_selection_state = m_selection_set.primary_selection;
    syncDocumentToViews(false);
    m_log_panel->appendMessage(
        LogSeverity::Info,
        QStringLiteral("[info] Added %1 to %2")
            .arg(toolModeLabel(m_workspace_tool_mode).toLower(), layerLogLabel(m_document_data, layerIndex)));
    statusBar()->showMessage(QStringLiteral("Primitive added"), 2000);
}

void MainWindow::cancelDrawing()
{
    clearDrawingDraft(true);
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(
        this,
        QStringLiteral("About PolyShow"),
        QStringLiteral("PolyShow MVP\n\n") + QStringLiteral("Features:\n")
            + QStringLiteral("1. Create, open, and export layers\n")
            + QStringLiteral("2. Display points, polylines, and polygons\n")
            + QStringLiteral("3. Draw primitives directly in the workspace\n")
            + QStringLiteral("4. Toggle layer and primitive visibility\n")
            + QStringLiteral("5. Search and inspect opened geometry\n")
            + QStringLiteral("6. Switch render mode (Solid/Wireframe/Points)"));
}

void MainWindow::startIpcListener()
{
    if (m_is_ipc_listener_running)
    {
        return;
    }

    m_ipc_thread = new QThread(this);
    m_ipc_listener_worker = new IpcListenerWorker();
    m_ipc_listener_worker->moveToThread(m_ipc_thread);

    connect(m_ipc_thread, &QThread::finished, m_ipc_listener_worker, &QObject::deleteLater);
    connect(
        m_ipc_listener_worker,
        &IpcListenerWorker::primitiveWriteRequested,
        this,
        &MainWindow::onIpcPrimitiveWriteRequested);
    connect(
        m_ipc_listener_worker,
        &IpcListenerWorker::protocolErrorLogged,
        this,
        &MainWindow::onIpcProtocolErrorLogged);
    connect(this, &MainWindow::ipcWriteProcessed, m_ipc_listener_worker, &IpcListenerWorker::deliverWriteResult);

    m_ipc_thread->start();

    bool started = false;
    QString errorMessage;
    QString listeningAddress = ipcNativeEndpoint();
    QMetaObject::invokeMethod(
        m_ipc_listener_worker,
        [this, &started, &errorMessage, &listeningAddress]() {
            started = m_ipc_listener_worker->startListening(&errorMessage);
            listeningAddress = m_ipc_listener_worker->listeningAddress();
        },
        Qt::BlockingQueuedConnection);

    if (!started)
    {
        shutdownIpcListener(false);
        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed to start IPC listener on %1: %2").arg(listeningAddress, errorMessage));
        statusBar()->showMessage(QStringLiteral("IPC listener failed to start"), 3000);
        return;
    }

    m_ipc_listening_address = listeningAddress;
    m_is_ipc_listener_running = true;
    updateIpcActionState();
    m_log_panel->appendMessage(
        LogSeverity::Info,
        QStringLiteral("[info] IPC listener started on %1").arg(m_ipc_listening_address));
    statusBar()->showMessage(QStringLiteral("IPC listener started on %1").arg(m_ipc_listening_address), 3000);
}

void MainWindow::stopIpcListener()
{
    shutdownIpcListener(true);
}

void MainWindow::onIpcPrimitiveWriteRequested(const IpcPrimitiveWriteMessage &message)
{
    IpcPrimitiveWriteResult result;
    result.sequence = message.sequence;
    result.request_id = message.request_id;
    result.layer_name = message.layer_name;
    result.primitive_name = message.request.name;

    QString resultMessage;
    bool replaced = false;
    if (!writePrimitiveToNamedIpcLayer(m_document_data, message.layer_name, message.request, &resultMessage, &replaced))
    {
        result.ok = false;
        result.message = resultMessage;
        emit ipcWriteProcessed(result);

        m_log_panel->appendMessage(
            LogSeverity::Error,
            QStringLiteral("[error] Failed IPC write to %1: %2").arg(message.layer_name, resultMessage));
        statusBar()->showMessage(QStringLiteral("IPC write failed"), 3000);
        return;
    }

    result.ok = true;
    result.message = resultMessage;
    result.result = replaced ? QStringLiteral("replaced") : QStringLiteral("appended");

    syncDocumentToViews(false);
    emit ipcWriteProcessed(result);

    if (message.request.name.isEmpty())
    {
        m_log_panel->appendMessage(
            LogSeverity::Info,
            QStringLiteral("[info] Appended IPC primitive to %1").arg(message.layer_name));
    }
    else if (replaced)
    {
        m_log_panel->appendMessage(
            LogSeverity::Warning,
            QStringLiteral("[warning] Replaced IPC primitive %1 in %2").arg(message.request.name, message.layer_name));
    }
    else
    {
        m_log_panel->appendMessage(
            LogSeverity::Info,
            QStringLiteral("[info] Appended IPC primitive %1 to %2").arg(message.request.name, message.layer_name));
    }

    statusBar()->showMessage(QStringLiteral("IPC write applied to %1").arg(message.layer_name), 3000);
}

void MainWindow::onIpcProtocolErrorLogged(const QString &message)
{
    m_log_panel->appendMessage(LogSeverity::Error, message);
    statusBar()->showMessage(QStringLiteral("IPC request rejected"), 3000);
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("PolyShow"));
    resize(1280, 800);
    setObjectName(QStringLiteral("mainWindow"));

    m_scene = new GeometryScene(this);
    m_geometry_viewer = new GeometryViewer(this);
    m_geometry_viewer->setObjectName(QStringLiteral("geometryViewport"));
    m_geometry_viewer->setScene(m_scene);

    m_layer_sidebar = new LayerSidebar(this);
    m_inspector_panel = new InspectorPanel(this);
    m_log_panel = new LogPanel(this);

    auto *leftContainer = new PanelFrame(PanelFrame::Variant::Panel, this);
    leftContainer->setObjectName(QStringLiteral("outlinerPanel"));
    leftContainer->setMinimumWidth(260);
    auto *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    leftLayout->addWidget(m_layer_sidebar);

    m_viewport_frame = new PanelFrame(PanelFrame::Variant::Canvas, this);
    m_viewport_frame->setObjectName(QStringLiteral("viewportPanel"));
    auto *viewportLayout = new QVBoxLayout(m_viewport_frame);
    viewportLayout->setContentsMargins(0, 0, 0, 0);
    viewportLayout->setSpacing(0);
    m_viewport_controls_widget = new QWidget(m_viewport_frame);
    m_viewport_controls_widget->setObjectName(QStringLiteral("viewportToolbar"));
    m_viewport_controls_widget->setAttribute(Qt::WA_StyledBackground, true);
    m_viewport_controls_widget->setFixedHeight(40);
    viewportLayout->addWidget(m_viewport_controls_widget);
    viewportLayout->addWidget(m_geometry_viewer, 1);

    m_inspector_container = new PanelFrame(PanelFrame::Variant::Panel, this);
    m_inspector_container->setObjectName(QStringLiteral("inspectorPanel"));
    m_inspector_container->setMinimumWidth(260);
    auto *inspectorLayout = new QVBoxLayout(m_inspector_container);
    inspectorLayout->setContentsMargins(0, 0, 0, 0);
    inspectorLayout->setSpacing(0);
    inspectorLayout->addWidget(m_inspector_panel);
    m_inspector_container->setVisible(false);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setObjectName(QStringLiteral("mainSplit"));
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(1);
    m_splitter->addWidget(leftContainer);
    m_splitter->addWidget(m_viewport_frame);
    m_splitter->addWidget(m_inspector_container);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setStretchFactor(2, 0);
    m_splitter->setSizes({304, 672, 0});

    m_log_tab_container = new PanelFrame(PanelFrame::Variant::Panel, this);
    m_log_tab_container->setObjectName(QStringLiteral("logPanelFrame"));
    auto *logLayout = new QVBoxLayout(m_log_tab_container);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->setSpacing(0);
    logLayout->addWidget(m_log_panel);
    m_log_tab_container->setMinimumHeight(104);

    m_vertical_splitter = new QSplitter(Qt::Vertical, this);
    m_vertical_splitter->setObjectName(QStringLiteral("workspaceSplit"));
    m_vertical_splitter->setChildrenCollapsible(false);
    m_vertical_splitter->setHandleWidth(1);
    m_vertical_splitter->addWidget(m_splitter);
    m_vertical_splitter->addWidget(m_log_tab_container);
    m_vertical_splitter->setStretchFactor(0, 1);
    m_vertical_splitter->setStretchFactor(1, 0);
    m_vertical_splitter->setSizes({640, 132});

    auto *centralWidget = new QWidget(this);
    centralWidget->setObjectName(QStringLiteral("workspaceRoot"));
    centralWidget->setAttribute(Qt::WA_StyledBackground, true);
    auto *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_topbar_widget = new QWidget(centralWidget);
    m_topbar_widget->setObjectName(QStringLiteral("topbar"));
    m_topbar_widget->setAttribute(Qt::WA_StyledBackground, true);
    m_topbar_widget->setFixedHeight(34);
    m_topbar_layout = new QHBoxLayout(m_topbar_widget);
    m_topbar_layout->setContentsMargins(12, 0, 12, 0);
    m_topbar_layout->setSpacing(12);
    layout->addWidget(m_topbar_widget);
    layout->addWidget(m_vertical_splitter, 1);
    setCentralWidget(centralWidget);

    connect(m_scene, &GeometryScene::geometryChanged, this, &MainWindow::onGeometryChanged);
    connect(m_geometry_viewer, &GeometryViewer::mousePositionChanged, this, &MainWindow::updateMousePosition);
    connect(m_geometry_viewer, &GeometryViewer::mousePositionChanged, this, &MainWindow::onWorkspaceHoverChanged);
    connect(m_geometry_viewer, &GeometryViewer::workspaceHoverExited, this, &MainWindow::onWorkspaceHoverExited);
    connect(m_geometry_viewer, &GeometryViewer::primitiveActivated, this, &MainWindow::onScenePrimitiveActivated);
    connect(m_geometry_viewer, &GeometryViewer::emptyAreaActivated, this, &MainWindow::onEmptySceneActivated);
    connect(m_geometry_viewer, &GeometryViewer::drawingPointRequested, this, &MainWindow::onDrawingPointRequested);
    connect(m_geometry_viewer, &GeometryViewer::drawingFinishedRequested, this, &MainWindow::finishDrawing);
    connect(m_layer_sidebar, &LayerSidebar::selectionRequested, this, &MainWindow::onSelectionRequested);
    connect(m_layer_sidebar, &LayerSidebar::layerVisibilityChanged, this, &MainWindow::onLayerVisibilityChanged);
    connect(m_layer_sidebar, &LayerSidebar::primitiveVisibilityChanged, this, &MainWindow::onPrimitiveVisibilityChanged);
    connect(m_layer_sidebar, &LayerSidebar::createLayerRequested, this, &MainWindow::createLayer);
    connect(m_layer_sidebar, &LayerSidebar::exportLayerRequested, this, &MainWindow::exportActiveLayer);
    connect(m_inspector_panel, &InspectorPanel::styleChangeRequested, this, &MainWindow::onInspectorStyleChangeRequested);
    connect(
        m_inspector_panel,
        &InspectorPanel::coordinateDraftChanged,
        this,
        &MainWindow::onInspectorCoordinateDraftChanged);
}

void MainWindow::setupMenuBar()
{
    menuBar()->setVisible(false);

    m_file_menu = menuBar()->addMenu(QStringLiteral("File"));

    m_new_layer_action = new QAction(materialIcon(QStringLiteral("add")), QStringLiteral("New Layer"), this);
    m_new_layer_action->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+N")));
    connect(m_new_layer_action, &QAction::triggered, this, &MainWindow::createLayer);
    m_file_menu->addAction(m_new_layer_action);

    m_open_action = new QAction(materialIcon(QStringLiteral("folder_open")), QStringLiteral("Open .ply..."), this);
    m_open_action->setShortcut(QKeySequence::Open);
    connect(m_open_action, &QAction::triggered, this, &MainWindow::openPlyFile);
    m_file_menu->addAction(m_open_action);

    m_export_layer_action =
        new QAction(materialIcon(QStringLiteral("ios_share")), QStringLiteral("Export Active Layer..."), this);
    m_export_layer_action->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    connect(m_export_layer_action, &QAction::triggered, this, &MainWindow::exportActiveLayer);
    m_file_menu->addAction(m_export_layer_action);

    m_file_menu->addSeparator();

    m_exit_action = new QAction(materialIcon(QStringLiteral("close")), QStringLiteral("Exit"), this);
    m_exit_action->setShortcut(QKeySequence::Quit);
    connect(m_exit_action, &QAction::triggered, this, &QMainWindow::close);
    m_file_menu->addAction(m_exit_action);

    m_view_menu = menuBar()->addMenu(QStringLiteral("View"));

    m_fit_action = new QAction(materialIcon(QStringLiteral("fit_screen")), QStringLiteral("Fit to View"), this);
    connect(m_fit_action, &QAction::triggered, m_geometry_viewer, &GeometryViewer::fitScene);
    m_view_menu->addAction(m_fit_action);

    m_zoom_in_action = new QAction(materialIcon(QStringLiteral("zoom_in")), QStringLiteral("Zoom In"), this);
    m_zoom_in_action->setShortcut(QKeySequence::ZoomIn);
    connect(m_zoom_in_action, &QAction::triggered, m_geometry_viewer, &GeometryViewer::zoomIn);
    m_view_menu->addAction(m_zoom_in_action);

    m_zoom_out_action = new QAction(materialIcon(QStringLiteral("zoom_out")), QStringLiteral("Zoom Out"), this);
    m_zoom_out_action->setShortcut(QKeySequence::ZoomOut);
    connect(m_zoom_out_action, &QAction::triggered, m_geometry_viewer, &GeometryViewer::zoomOut);
    m_view_menu->addAction(m_zoom_out_action);

    m_reset_view_action =
        new QAction(materialIcon(QStringLiteral("center_focus_strong")), QStringLiteral("Reset View"), this);
    connect(m_reset_view_action, &QAction::triggered, m_geometry_viewer, &GeometryViewer::resetViewTransform);
    m_view_menu->addAction(m_reset_view_action);

    m_render_menu = menuBar()->addMenu(QStringLiteral("Render"));
    m_render_mode_action_group = new QActionGroup(this);
    m_render_mode_action_group->setExclusive(true);

    m_solid_mode_action = new QAction(materialIcon(QStringLiteral("deployed_code")), QStringLiteral("Solid"), this);
    m_solid_mode_action->setCheckable(true);
    connect(m_solid_mode_action, &QAction::triggered, this, &MainWindow::setRenderModeSolid);
    m_render_mode_action_group->addAction(m_solid_mode_action);
    m_render_menu->addAction(m_solid_mode_action);

    m_wireframe_mode_action = new QAction(materialIcon(QStringLiteral("polyline")), QStringLiteral("Wireframe"), this);
    m_wireframe_mode_action->setCheckable(true);
    connect(m_wireframe_mode_action, &QAction::triggered, this, &MainWindow::setRenderModeWireframe);
    m_render_mode_action_group->addAction(m_wireframe_mode_action);
    m_render_menu->addAction(m_wireframe_mode_action);

    m_points_mode_action =
        new QAction(materialIcon(QStringLiteral("radio_button_checked")), QStringLiteral("Points"), this);
    m_points_mode_action->setCheckable(true);
    connect(m_points_mode_action, &QAction::triggered, this, &MainWindow::setRenderModePoints);
    m_render_mode_action_group->addAction(m_points_mode_action);
    m_render_menu->addAction(m_points_mode_action);

    m_ipc_menu = menuBar()->addMenu(QStringLiteral("IPC"));

    m_start_ipc_listener_action =
        new QAction(materialIcon(QStringLiteral("sensors")), QStringLiteral("Start IPC Listener"), this);
    connect(m_start_ipc_listener_action, &QAction::triggered, this, &MainWindow::startIpcListener);
    m_ipc_menu->addAction(m_start_ipc_listener_action);

    m_stop_ipc_listener_action =
        new QAction(materialIcon(QStringLiteral("sensors_off")), QStringLiteral("Stop IPC Listener"), this);
    connect(m_stop_ipc_listener_action, &QAction::triggered, this, &MainWindow::stopIpcListener);
    m_ipc_menu->addAction(m_stop_ipc_listener_action);

    m_help_menu = menuBar()->addMenu(QStringLiteral("Help"));
    m_about_action = new QAction(materialIcon(QStringLiteral("info")), QStringLiteral("About"), this);
    connect(m_about_action, &QAction::triggered, this, &MainWindow::showAboutDialog);
    m_help_menu->addAction(m_about_action);

    addActions(
        {m_new_layer_action,
         m_open_action,
         m_export_layer_action,
         m_exit_action,
         m_zoom_in_action,
         m_zoom_out_action});

    updateIpcActionState();
}

void MainWindow::setupTopBar()
{
    if (m_topbar_layout == nullptr)
    {
        return;
    }

    auto *brand = new QWidget(m_topbar_widget);
    brand->setObjectName(QStringLiteral("brand"));
    brand->setAttribute(Qt::WA_StyledBackground, true);
    auto *brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(8, 0, 8, 0);
    brandLayout->setSpacing(6);

    auto *logoLabel = new QLabel(brand);
    logoLabel->setObjectName(QStringLiteral("brandLogo"));
    logoLabel->setPixmap(QIcon(QStringLiteral(":/icons/polyshow-logo.svg")).pixmap(18, 18));
    brandLayout->addWidget(logoLabel);

    auto *brandText = new QLabel(QStringLiteral("PolyShow"), brand);
    brandText->setObjectName(QStringLiteral("brandText"));
    brandLayout->addWidget(brandText);
    m_topbar_layout->addWidget(brand);

    auto *menuStrip = new QWidget(m_topbar_widget);
    menuStrip->setObjectName(QStringLiteral("menuStrip"));
    auto *menuLayout = new QHBoxLayout(menuStrip);
    menuLayout->setContentsMargins(0, 0, 0, 0);
    menuLayout->setSpacing(14);

    const auto addMenuButton = [menuStrip, menuLayout](const QString &text, QMenu *menu) {
        auto *button = new QToolButton(menuStrip);
        button->setObjectName(QStringLiteral("menuButton"));
        button->setText(text);
        button->setPopupMode(QToolButton::InstantPopup);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setMenu(menu);
        button->setCursor(Qt::PointingHandCursor);
        menuLayout->addWidget(button);
    };

    addMenuButton(QStringLiteral("File"), m_file_menu);
    addMenuButton(QStringLiteral("View"), m_view_menu);
    addMenuButton(QStringLiteral("Render"), m_render_menu);
    addMenuButton(QStringLiteral("IPC"), m_ipc_menu);
    addMenuButton(QStringLiteral("Help"), m_help_menu);
    m_topbar_layout->addWidget(menuStrip);
    m_topbar_layout->addStretch();

    auto *tabs = new QWidget(m_topbar_widget);
    tabs->setObjectName(QStringLiteral("workspaceTabs"));
    auto *tabsLayout = new QHBoxLayout(tabs);
    tabsLayout->setContentsMargins(0, 0, 0, 0);
    tabsLayout->setSpacing(4);

    const QStringList tabNames {QStringLiteral("Modeling"), QStringLiteral("Inspect"), QStringLiteral("IPC")};
    for (int index = 0; index < tabNames.size(); ++index)
    {
        auto *tab = new QPushButton(tabNames.at(index), tabs);
        tab->setObjectName(index == 0 ? QStringLiteral("activeWorkspaceTab") : QStringLiteral("workspaceTab"));
        tab->setCursor(Qt::PointingHandCursor);
        tab->setFlat(true);
        tabsLayout->addWidget(tab);
    }

    m_topbar_layout->addWidget(tabs);
}

void MainWindow::setupStatusBar()
{
    statusBar()->setObjectName(QStringLiteral("statusbar"));
    statusBar()->setAttribute(Qt::WA_StyledBackground, true);
    statusBar()->setFixedHeight(26);
    statusBar()->setSizeGripEnabled(false);

    m_status_info_label = new QLabel(QStringLiteral("Points: 0  Polylines: 0  Polygons: 0"), this);
    m_status_info_label->setObjectName(QStringLiteral("statusInfo"));
    m_status_mouse_label = new QLabel(QStringLiteral("X: 0.00  Y: 0.00"), this);
    m_status_mouse_label->setObjectName(QStringLiteral("statusMouse"));

    statusBar()->addPermanentWidget(m_status_info_label);
    statusBar()->addPermanentWidget(m_status_mouse_label);
    statusBar()->showMessage(QStringLiteral("Ready"), 1500);
}

void MainWindow::setupViewportControls()
{
    auto *layout = new QHBoxLayout(m_viewport_controls_widget);
    layout->setContentsMargins(6, 0, 6, 0);
    layout->setSpacing(4);

    m_browse_mode_button = new PillButton(QStringLiteral("Browse"), m_viewport_controls_widget);
    m_browse_mode_button->setIconName(QStringLiteral("near_me"));
    m_browse_mode_button->setCheckable(true);
    layout->addWidget(m_browse_mode_button);
    connect(m_browse_mode_button, &QPushButton::clicked, this, &MainWindow::setToolModeBrowse);

    m_draw_point_button = new PillButton(QStringLiteral("Point"), m_viewport_controls_widget);
    m_draw_point_button->setIconName(QStringLiteral("radio_button_checked"));
    m_draw_point_button->setCheckable(true);
    layout->addWidget(m_draw_point_button);
    connect(m_draw_point_button, &QPushButton::clicked, this, &MainWindow::setToolModeDrawPoint);

    m_draw_polyline_button = new PillButton(QStringLiteral("Line"), m_viewport_controls_widget);
    m_draw_polyline_button->setIconName(QStringLiteral("timeline"));
    m_draw_polyline_button->setCheckable(true);
    layout->addWidget(m_draw_polyline_button);
    connect(m_draw_polyline_button, &QPushButton::clicked, this, &MainWindow::setToolModeDrawPolyline);

    m_draw_polygon_button = new PillButton(QStringLiteral("Poly"), m_viewport_controls_widget);
    m_draw_polygon_button->setIconName(QStringLiteral("pentagon"));
    m_draw_polygon_button->setCheckable(true);
    layout->addWidget(m_draw_polygon_button);
    connect(m_draw_polygon_button, &QPushButton::clicked, this, &MainWindow::setToolModeDrawPolygon);

    m_finish_drawing_button = new PillButton(QStringLiteral("Done"), m_viewport_controls_widget);
    m_finish_drawing_button->setIconName(QStringLiteral("done"));
    layout->addWidget(m_finish_drawing_button);
    connect(m_finish_drawing_button, &QPushButton::clicked, this, &MainWindow::finishDrawing);

    m_cancel_drawing_button = new PillButton(QString(), m_viewport_controls_widget);
    m_cancel_drawing_button->setIconName(QStringLiteral("close"));
    m_cancel_drawing_button->setToolTip(QStringLiteral("Cancel Drawing"));
    layout->addWidget(m_cancel_drawing_button);
    connect(m_cancel_drawing_button, &QPushButton::clicked, this, &MainWindow::cancelDrawing);

    layout->addStretch();

    auto *fitButton = new PillButton(QString(), m_viewport_controls_widget);
    fitButton->setIconName(QStringLiteral("fit_screen"));
    fitButton->setToolTip(QStringLiteral("Fit View"));
    layout->addWidget(fitButton);
    connect(fitButton, &QPushButton::clicked, m_fit_action, &QAction::trigger);

    auto *zoomOutButton = new PillButton(QString(), m_viewport_controls_widget);
    zoomOutButton->setIconName(QStringLiteral("zoom_out"));
    zoomOutButton->setToolTip(QStringLiteral("Zoom Out"));
    layout->addWidget(zoomOutButton);
    connect(zoomOutButton, &QPushButton::clicked, m_zoom_out_action, &QAction::trigger);

    auto *zoomInButton = new PillButton(QString(), m_viewport_controls_widget);
    zoomInButton->setIconName(QStringLiteral("zoom_in"));
    zoomInButton->setToolTip(QStringLiteral("Zoom In"));
    layout->addWidget(zoomInButton);
    connect(zoomInButton, &QPushButton::clicked, m_zoom_in_action, &QAction::trigger);

    auto *resetButton = new PillButton(QString(), m_viewport_controls_widget);
    resetButton->setIconName(QStringLiteral("center_focus_strong"));
    resetButton->setToolTip(QStringLiteral("Reset View"));
    layout->addWidget(resetButton);
    connect(resetButton, &QPushButton::clicked, m_reset_view_action, &QAction::trigger);

    m_grid_toggle_button = new PillButton(QString(), m_viewport_controls_widget);
    m_grid_toggle_button->setIconName(QStringLiteral("grid_on"));
    m_grid_toggle_button->setCheckable(true);
    m_grid_toggle_button->setToolTip(QStringLiteral("Toggle Grid"));
    layout->addWidget(m_grid_toggle_button);
    connect(m_grid_toggle_button, &QPushButton::clicked, this, [this](bool checked) {
        m_scene->setGridVisible(checked);
        updateViewportControlState();
    });

    m_render_mode_combo_box = new QComboBox(m_viewport_controls_widget);
    m_render_mode_combo_box->setObjectName(QStringLiteral("renderModeCombo"));
    m_render_mode_combo_box->addItem(materialIcon(QStringLiteral("deployed_code")), QStringLiteral("Solid"));
    m_render_mode_combo_box->addItem(materialIcon(QStringLiteral("polyline")), QStringLiteral("Wireframe"));
    m_render_mode_combo_box->addItem(materialIcon(QStringLiteral("radio_button_checked")), QStringLiteral("Points"));
    connect(
        m_render_mode_combo_box,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &MainWindow::onRenderModeChanged);
    layout->addWidget(m_render_mode_combo_box);

    updateViewportControlState();
    updateDrawingToolState();
}

void MainWindow::updateViewportControlState()
{
    if (m_grid_toggle_button != nullptr)
    {
        const QSignalBlocker blocker(m_grid_toggle_button);
        m_grid_toggle_button->setChecked(m_scene->isGridVisible());
        m_grid_toggle_button->setText(QString());
        m_grid_toggle_button->setVariant(
            m_scene->isGridVisible() ? PillButton::Variant::Primary : PillButton::Variant::Neutral);
        m_grid_toggle_button->setToolTip(
            m_scene->isGridVisible() ? QStringLiteral("Hide Grid") : QStringLiteral("Show Grid"));
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

void MainWindow::updateDrawingToolState()
{
    const auto syncButton = [this](PillButton *button, bool checked) {
        if (button == nullptr)
        {
            return;
        }

        const QSignalBlocker blocker(button);
        button->setChecked(checked);
        button->setVariant(checked ? PillButton::Variant::Primary : PillButton::Variant::Neutral);
    };

    syncButton(m_browse_mode_button, m_workspace_tool_mode == GeometryViewer::ToolMode::Browse);
    syncButton(m_draw_point_button, m_workspace_tool_mode == GeometryViewer::ToolMode::DrawPoint);
    syncButton(m_draw_polyline_button, m_workspace_tool_mode == GeometryViewer::ToolMode::DrawPolyline);
    syncButton(m_draw_polygon_button, m_workspace_tool_mode == GeometryViewer::ToolMode::DrawPolygon);

    const bool hasDraft = !m_drawing_points.isEmpty();
    const bool canFinish = m_workspace_tool_mode == GeometryViewer::ToolMode::DrawPolyline
        || m_workspace_tool_mode == GeometryViewer::ToolMode::DrawPolygon;
    if (m_finish_drawing_button != nullptr)
    {
        m_finish_drawing_button->setEnabled(canFinish && hasDraft);
        m_finish_drawing_button->setVariant(PillButton::Variant::Neutral);
    }

    if (m_cancel_drawing_button != nullptr)
    {
        m_cancel_drawing_button->setEnabled(hasDraft);
        m_cancel_drawing_button->setVariant(hasDraft ? PillButton::Variant::Neutral : PillButton::Variant::Neutral);
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

void MainWindow::setWorkspaceToolMode(GeometryViewer::ToolMode toolMode)
{
    if (m_workspace_tool_mode == toolMode)
    {
        updateDrawingPreview();
        updateDrawingToolState();
        return;
    }

    if (toolMode != m_workspace_tool_mode)
    {
        clearDrawingDraft(false);
    }

    m_workspace_tool_mode = toolMode;
    if (toolMode == GeometryViewer::ToolMode::Browse)
    {
        m_has_drawing_hover_point = false;
    }
    m_geometry_viewer->setToolMode(toolMode);
    updateDrawingPreview();
    updateDrawingToolState();

    if (toolMode == GeometryViewer::ToolMode::Browse)
    {
        statusBar()->showMessage(QStringLiteral("Browse mode"), 1500);
        return;
    }

    statusBar()->showMessage(
        toolMode == GeometryViewer::ToolMode::DrawPoint
            ? QStringLiteral("Point mode: left click to add a point")
            : QStringLiteral("%1 mode: left click to add vertices, Finish or right click to commit")
                  .arg(toolModeLabel(toolMode)),
        4000);
}

void MainWindow::openFiles(const QStringList &filePaths)
{
    DocumentData nextDocument = m_document_data;
    QStringList failureMessages;
    int openedCount = 0;

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

        const QString importedName = normalizedLayerBaseName(QFileInfo(filePath).fileName());
        const QString uniqueLayerName = uniqueLayerNameForDocument(nextDocument, importedName);
        if (uniqueLayerName != importedName)
        {
            m_log_panel->appendMessage(
                LogSeverity::Warning,
                QStringLiteral("[warning] Renamed imported layer %1 to %2 to keep layer names unique")
                    .arg(importedName, uniqueLayerName));
        }

        nextDocument.layers.append(buildLayerData(filePath, uniqueLayerName, geometryData));
        ++openedCount;
        m_log_panel->appendMessage(
            LogSeverity::Info,
            QStringLiteral("[info] %1 opened successfully").arg(uniqueLayerName));
    }

    if (openedCount == 0)
    {
        const QString message = failureMessages.isEmpty()
            ? QStringLiteral("No files were opened.")
            : failureMessages.join(QStringLiteral("\n\n"));
        QMessageBox::critical(this, QStringLiteral("Open Failed"), message);
        statusBar()->showMessage(QStringLiteral("Open failed"), 3000);
        return;
    }

    m_document_data = nextDocument;
    m_active_layer_index = m_document_data.layers.isEmpty() ? -1 : (m_document_data.layers.size() - 1);
    m_selection_set = {};
    m_selection_state = {};
    syncDocumentToViews(true);

    if (failureMessages.isEmpty())
    {
        statusBar()->showMessage(QStringLiteral("Opened %1 file(s)").arg(openedCount), 3000);
        return;
    }

    m_log_panel->appendMessage(
        LogSeverity::Warning,
        QStringLiteral("[warning] Opened %1 file(s), %2 failed").arg(openedCount).arg(failureMessages.size()));
    QMessageBox::warning(
        this,
        QStringLiteral("Open Completed with Errors"),
        QStringLiteral("Opened %1 file(s). %2 file(s) failed.\n\n%3")
            .arg(openedCount)
            .arg(failureMessages.size())
            .arg(failureMessages.join(QStringLiteral("\n\n"))));
    statusBar()->showMessage(
        QStringLiteral("Opened %1 file(s), %2 failed").arg(openedCount).arg(failureMessages.size()),
        4000);
}

void MainWindow::syncDocumentToViews(bool fitScene)
{
    clearCoordinatePreviewState();
    m_selection_set = normalizedSelectionSet(m_selection_set);
    m_selection_state = m_selection_set.primary_selection;
    m_layer_sidebar->setDocumentData(m_document_data, true);
    m_scene->setEditPreviewState(m_edit_preview_state, false);
    m_scene->setDocumentData(m_document_data);
    updateDrawingPreview();
    setSelectionSet(m_selection_set);

    if (fitScene)
    {
        m_geometry_viewer->fitScene();
    }
}

SelectionState MainWindow::normalizedSelectionState(const SelectionState &selectionState) const
{
    return normalizedSelection(m_document_data, selectionState);
}

SelectionSet MainWindow::normalizedSelectionSet(const SelectionSet &selectionSet) const
{
    return normalizedSelectionSetForDocument(m_document_data, selectionSet);
}

void MainWindow::setSelectionSet(const SelectionSet &selectionSet)
{
    const SelectionSet normalizedSelectionSetValue = normalizedSelectionSet(selectionSet);
    if (normalizedSelectionSetValue != m_selection_set)
    {
        clearCoordinatePreviewState();
    }

    m_selection_set = normalizedSelectionSetValue;
    m_selection_state = m_selection_set.primary_selection;
    if (m_selection_state.kind == SelectionKind::Layer || m_selection_state.kind == SelectionKind::Primitive)
    {
        m_active_layer_index = m_selection_state.layer_index;
    }

    m_scene->setEditPreviewState(m_edit_preview_state, false);
    m_scene->setSelectionSet(m_selection_set);
    m_layer_sidebar->setSelectionSet(m_selection_set);
    reloadInspectorForSelectionChange();

    const bool showInspector = m_selection_state.kind != SelectionKind::None
        || !m_selection_set.selected_primitives.isEmpty();
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

void MainWindow::refreshViewsForVisibilityChange()
{
    // Keep checkbox-triggered updates on the existing outliner model shape; the
    // source document remains the single owner of final visibility state.
    clearCoordinatePreviewState();
    const SelectionSet nextSelectionSet = normalizedSelectionSet(m_selection_set);
    m_layer_sidebar->setDocumentData(m_document_data, false);
    m_scene->setEditPreviewState(m_edit_preview_state, false);
    m_scene->setDocumentData(m_document_data);
    updateDrawingPreview();
    setSelectionSet(nextSelectionSet);
}

void MainWindow::reloadInspectorForSelectionChange()
{
    m_inspector_panel->loadSelectionContext(m_document_data, m_selection_set);
}

void MainWindow::refreshSceneForPrimitiveEdit()
{
    m_scene->setEditPreviewState(m_edit_preview_state, false);
    m_scene->setDocumentData(m_document_data);
    updateDrawingPreview();
}

void MainWindow::clearCoordinatePreviewState()
{
    m_edit_preview_state = PrimitiveEditPreviewState {};
    m_has_logged_invalid_coordinate_draft = false;
    m_scene->setEditPreviewState(m_edit_preview_state, false);
    m_inspector_panel->clearCoordinateError();
}

void MainWindow::clearDrawingDraft(bool showStatusMessage)
{
    if (m_drawing_points.isEmpty())
    {
        updateDrawingPreview();
        updateDrawingToolState();
        return;
    }

    m_drawing_points.clear();
    updateDrawingPreview();
    updateDrawingToolState();
    if (showStatusMessage)
    {
        statusBar()->showMessage(QStringLiteral("Drawing canceled"), 2000);
    }
}

void MainWindow::updateDrawingPreview()
{
    if (m_workspace_tool_mode == GeometryViewer::ToolMode::Browse)
    {
        m_scene->clearDrawingPreview();
        updateDrawingToolState();
        return;
    }

    QVector<Point2D> previewPoints = m_drawing_points;
    if (m_has_drawing_hover_point)
    {
        if (previewPoints.isEmpty() || !pointsEqual(previewPoints.last(), m_drawing_hover_point))
        {
            previewPoints.append(m_drawing_hover_point);
        }
    }

    if (previewPoints.isEmpty())
    {
        m_scene->clearDrawingPreview();
        updateDrawingToolState();
        return;
    }

    m_scene->setDrawingPreview(primitiveKindForToolMode(m_workspace_tool_mode), previewPoints);
    updateDrawingToolState();
}

int MainWindow::currentLayerIndex() const
{
    if (m_selection_state.kind == SelectionKind::Layer || m_selection_state.kind == SelectionKind::Primitive)
    {
        if (m_selection_state.layer_index >= 0 && m_selection_state.layer_index < m_document_data.layers.size())
        {
            return m_selection_state.layer_index;
        }
    }

    if (m_active_layer_index >= 0 && m_active_layer_index < m_document_data.layers.size())
    {
        return m_active_layer_index;
    }

    return -1;
}

int MainWindow::ensureActiveLayer()
{
    const int existingLayerIndex = currentLayerIndex();
    if (existingLayerIndex >= 0)
    {
        return existingLayerIndex;
    }

    if (!m_document_data.layers.isEmpty())
    {
        m_active_layer_index = m_document_data.layers.size() - 1;
        return m_active_layer_index;
    }

    createLayer();
    return currentLayerIndex();
}

QString MainWindow::nextLayerName() const
{
    for (int index = 1;; ++index)
    {
        const QString candidate = QStringLiteral("Layer %1").arg(index);
        if (findLayerByName(m_document_data, candidate) == nullptr)
        {
            return candidate;
        }
    }
}

void MainWindow::updateIpcActionState()
{
    if (m_start_ipc_listener_action != nullptr)
    {
        m_start_ipc_listener_action->setEnabled(!m_is_ipc_listener_running);
    }

    if (m_stop_ipc_listener_action != nullptr)
    {
        m_stop_ipc_listener_action->setEnabled(m_is_ipc_listener_running);
    }
}

void MainWindow::shutdownIpcListener(bool showUserFeedback)
{
    if (m_ipc_thread == nullptr || m_ipc_listener_worker == nullptr)
    {
        m_is_ipc_listener_running = false;
        m_ipc_listening_address.clear();
        updateIpcActionState();
        return;
    }

    QThread *thread = m_ipc_thread;
    IpcListenerWorker *worker = m_ipc_listener_worker;
    const bool wasRunning = m_is_ipc_listener_running;

    QMetaObject::invokeMethod(
        worker,
        [worker]() {
            worker->stopListening();
        },
        Qt::BlockingQueuedConnection);

    thread->quit();
    thread->wait();

    m_ipc_listener_worker = nullptr;
    m_ipc_thread = nullptr;
    m_is_ipc_listener_running = false;
    updateIpcActionState();

    thread->deleteLater();

    if (showUserFeedback && wasRunning)
    {
        m_log_panel->appendMessage(
            LogSeverity::Info,
            QStringLiteral("[info] IPC listener stopped on %1").arg(m_ipc_listening_address));
        statusBar()->showMessage(QStringLiteral("IPC listener stopped"), 3000);
    }

    m_ipc_listening_address.clear();
}

void MainWindow::updateUiFromScene()
{
    setRenderMode(m_scene->renderMode());
    m_geometry_viewer->setToolMode(m_workspace_tool_mode);
    onGeometryChanged(m_scene->pointCount(), m_scene->polylineCount(), m_scene->polygonCount());
    updateViewportControlState();
    updateDrawingToolState();
    setSelectionSet(m_selection_set);
}

} // namespace PolyShow
