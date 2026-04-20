#include "ui/InspectorPanel.h"

#include "core/PrimitiveEditing.h"
#include "ui/PanelFrame.h"

#include <QCheckBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRectF>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

namespace PolyShow
{

namespace
{

/// Returns the badge text for one selection kind.
QString selectionBadgeText(SelectionKind kind)
{
    switch (kind)
    {
    case SelectionKind::Layer:
        return QStringLiteral("Layer");
    case SelectionKind::Primitive:
        return QStringLiteral("Primitive");
    default:
        return QStringLiteral("None");
    }
}

/// Returns the label for one primitive kind.
QString primitiveKindText(PrimitiveKind kind)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        return QStringLiteral("Point");
    case PrimitiveKind::Polyline:
        return QStringLiteral("Polyline");
    case PrimitiveKind::Polygon:
        return QStringLiteral("Polygon");
    default:
        return QStringLiteral("Unknown");
    }
}

/// Computes the axis-aligned bounds of one point list.
QRectF pointsBounds(const QVector<Point2D> &points)
{
    if (points.isEmpty())
    {
        return {};
    }

    double minX = points.first().x;
    double maxX = points.first().x;
    double minY = points.first().y;
    double maxY = points.first().y;

    for (const Point2D &point : points)
    {
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }

    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

/// Summarizes one layer for read-only display.
QString layerSummaryText(const LayerData &layer)
{
    int visibleCount = 0;
    int polygonCount = 0;
    int polylineCount = 0;
    int pointCount = 0;

    for (const LayerPrimitiveData &primitive : layer.primitives)
    {
        if (primitive.visible)
        {
            ++visibleCount;
        }

        switch (primitive.reference.kind)
        {
        case PrimitiveKind::Point:
            ++pointCount;
            break;
        case PrimitiveKind::Polyline:
            ++polylineCount;
            break;
        case PrimitiveKind::Polygon:
            ++polygonCount;
            break;
        }
    }

    return QStringLiteral(
               "Primitives   %1\nVisible      %2\nHidden       %3\nPolygons     %4\nPolylines    %5\nPoints       %6")
        .arg(layer.primitives.size())
        .arg(visibleCount)
        .arg(layer.primitives.size() - visibleCount)
        .arg(polygonCount)
        .arg(polylineCount)
        .arg(pointCount);
}

/// Summarizes one selected primitive for read-only display.
QString primitiveGeometryText(const LayerData &layer, int primitiveIndex)
{
    const LayerPrimitiveData &primitive = layer.primitives.at(primitiveIndex);
    const QVector<Point2D> points = primitivePoints(layer, primitiveIndex);
    const QRectF bounds = pointsBounds(points);

    return QStringLiteral("Type        %1\nVertices    %2\nBounds      %3 x %4\nVisible     %5")
        .arg(primitiveKindText(primitive.reference.kind))
        .arg(points.size())
        .arg(bounds.width(), 0, 'f', 2)
        .arg(bounds.height(), 0, 'f', 2)
        .arg(primitive.visible ? QStringLiteral("true") : QStringLiteral("false"));
}

/// Creates a section label for the inspector layout.
QLabel *createSectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("role", QStringLiteral("sectionTitle"));
    return label;
}

/// Creates a compact validation label hidden by default.
QLabel *createErrorLabel(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setProperty("role", QStringLiteral("validationError"));
    label->setWordWrap(true);
    label->setVisible(false);
    return label;
}

/// Refreshes a widget after its validation property changes.
void refreshValidationStyle(QWidget *widget)
{
    if (widget == nullptr)
    {
        return;
    }

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

/// Creates a titled field section that can be hidden as one block.
QWidget *createFieldSection(
    QWidget *parent, const QString &title, QWidget *editor, QLabel **errorLabel, QLabel **titleLabel = nullptr)
{
    auto *section = new QWidget(parent);
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    QLabel *fieldTitle = createSectionTitle(title, section);
    layout->addWidget(fieldTitle);
    layout->addWidget(editor);

    QLabel *fieldErrorLabel = createErrorLabel(section);
    layout->addWidget(fieldErrorLabel);

    if (errorLabel != nullptr)
    {
        *errorLabel = fieldErrorLabel;
    }

    if (titleLabel != nullptr)
    {
        *titleLabel = fieldTitle;
    }

    return section;
}

} // namespace

InspectorPanel::InspectorPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    auto *panelTitle = new QLabel(QStringLiteral("Inspector"), this);
    panelTitle->setProperty("role", QStringLiteral("panelTitle"));
    headerLayout->addWidget(panelTitle);
    headerLayout->addStretch();

    m_badge_label = new QLabel(this);
    m_badge_label->setObjectName(QStringLiteral("inspectorBadge"));
    headerLayout->addWidget(m_badge_label);
    layout->addLayout(headerLayout);

    auto *card = new PanelFrame(PanelFrame::Variant::Card, this);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 12, 12, 12);
    cardLayout->setSpacing(10);

    m_title_label = new QLabel(card);
    m_title_label->setProperty("role", QStringLiteral("panelTitle"));
    cardLayout->addWidget(m_title_label);

    m_meta_label = new QLabel(card);
    m_meta_label->setWordWrap(true);
    cardLayout->addWidget(m_meta_label);

    m_geometry_label = createSectionTitle(QStringLiteral("Geometry"), card);
    cardLayout->addWidget(m_geometry_label);

    m_geometry_body_label = new QLabel(card);
    m_geometry_body_label->setProperty("role", QStringLiteral("mono"));
    m_geometry_body_label->setTextFormat(Qt::PlainText);
    m_geometry_body_label->setWordWrap(true);
    cardLayout->addWidget(m_geometry_body_label);

    m_editor_widget = new QWidget(card);
    auto *editorLayout = new QVBoxLayout(m_editor_widget);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(8);

    m_edit_state_label = new QLabel(m_editor_widget);
    m_edit_state_label->setWordWrap(true);
    editorLayout->addWidget(m_edit_state_label);

    m_stroke_color_line_edit = new QLineEdit(m_editor_widget);
    m_stroke_color_line_edit->setPlaceholderText(QStringLiteral("#RRGGBB or #RRGGBBAA"));
    editorLayout->addWidget(
        createFieldSection(m_editor_widget, QStringLiteral("Stroke Color"), m_stroke_color_line_edit, &m_stroke_color_error_label));

    auto *fillWrapper = new QWidget(m_editor_widget);
    auto *fillLayout = new QVBoxLayout(fillWrapper);
    fillLayout->setContentsMargins(0, 0, 0, 0);
    fillLayout->setSpacing(6);
    m_fill_enabled_check_box = new QCheckBox(QStringLiteral("Enable Fill"), fillWrapper);
    fillLayout->addWidget(m_fill_enabled_check_box);
    m_fill_color_line_edit = new QLineEdit(fillWrapper);
    m_fill_color_line_edit->setPlaceholderText(QStringLiteral("#RRGGBB or #RRGGBBAA"));
    fillLayout->addWidget(
        createFieldSection(fillWrapper, QStringLiteral("Fill Color"), m_fill_color_line_edit, &m_fill_color_error_label));
    m_fill_section_widget = createFieldSection(m_editor_widget, QStringLiteral("Fill"), fillWrapper, nullptr);
    editorLayout->addWidget(m_fill_section_widget);

    m_width_line_edit = new QLineEdit(m_editor_widget);
    m_width_line_edit->setPlaceholderText(QStringLiteral("Greater than 0"));
    m_width_section_widget =
        createFieldSection(m_editor_widget, QStringLiteral("Line Width"), m_width_line_edit, &m_width_error_label);
    editorLayout->addWidget(m_width_section_widget);

    m_point_size_line_edit = new QLineEdit(m_editor_widget);
    m_point_size_line_edit->setPlaceholderText(QStringLiteral("Greater than 0"));
    editorLayout->addWidget(
        createFieldSection(m_editor_widget, QStringLiteral("Point Size"), m_point_size_line_edit, &m_point_size_error_label));

    m_coordinates_text_edit = new QPlainTextEdit(m_editor_widget);
    m_coordinates_text_edit->setPlaceholderText(QStringLiteral("x y"));
    m_coordinates_text_edit->setMinimumHeight(132);
    m_coordinates_text_edit->setProperty("role", QStringLiteral("mono"));
    m_coordinates_text_edit->setFont(QFont(QStringLiteral("IBM Plex Mono"), 10));
    editorLayout->addWidget(
        createFieldSection(m_editor_widget, QStringLiteral("Coordinates"), m_coordinates_text_edit, &m_coordinates_error_label));

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);
    buttonLayout->addStretch();

    m_reset_button = new QPushButton(QStringLiteral("Reset"), m_editor_widget);
    buttonLayout->addWidget(m_reset_button);

    m_apply_button = new QPushButton(QStringLiteral("Apply"), m_editor_widget);
    buttonLayout->addWidget(m_apply_button);
    editorLayout->addLayout(buttonLayout);

    cardLayout->addWidget(m_editor_widget);

    m_hint_label = new QLabel(card);
    m_hint_label->setWordWrap(true);
    cardLayout->addWidget(m_hint_label);

    cardLayout->addStretch();
    layout->addWidget(card);

    const auto connectEditorChanged = [this]() {
        if (!m_is_loading_form)
        {
            updateEditorState();
        }
    };

    connect(m_stroke_color_line_edit, &QLineEdit::textChanged, this, [connectEditorChanged](const QString &) {
        connectEditorChanged();
    });
    connect(m_fill_enabled_check_box, &QCheckBox::toggled, this, [this](bool) {
        if (!m_is_loading_form)
        {
            updateVisibleEditorFields(currentEditRequest().primitive_kind);
            updateEditorState();
        }
    });
    connect(m_fill_color_line_edit, &QLineEdit::textChanged, this, [connectEditorChanged](const QString &) {
        connectEditorChanged();
    });
    connect(m_width_line_edit, &QLineEdit::textChanged, this, [connectEditorChanged](const QString &) {
        connectEditorChanged();
    });
    connect(m_point_size_line_edit, &QLineEdit::textChanged, this, [connectEditorChanged](const QString &) {
        connectEditorChanged();
    });
    connect(m_coordinates_text_edit, &QPlainTextEdit::textChanged, this, [connectEditorChanged]() {
        connectEditorChanged();
    });

    connect(m_apply_button, &QPushButton::clicked, this, [this]() {
        emit applyRequested(currentEditRequest());
    });
    connect(m_reset_button, &QPushButton::clicked, this, [this]() {
        reloadCurrentPrimitiveEditor();
        emit resetRequested();
    });

    updateContent();
}

void InspectorPanel::setDocumentData(const DocumentData &documentData)
{
    m_document_data = documentData;
    updateContent();
}

void InspectorPanel::setSelectionState(const SelectionState &selectionState)
{
    m_selection_state = selectionState;
    updateContent();
}

void InspectorPanel::setValidationErrors(const PrimitiveEditValidationErrors &errors)
{
    setFieldError(m_stroke_color_line_edit, m_stroke_color_error_label, errors.stroke_color);
    setFieldError(m_fill_color_line_edit, m_fill_color_error_label, errors.fill_color);
    setFieldError(m_width_line_edit, m_width_error_label, errors.width);
    setFieldError(m_point_size_line_edit, m_point_size_error_label, errors.point_size);
    setFieldError(m_coordinates_text_edit, m_coordinates_error_label, errors.coordinates);
}

void InspectorPanel::clearValidationErrors()
{
    setValidationErrors(PrimitiveEditValidationErrors {});
}

void InspectorPanel::updateContent()
{
    m_badge_label->setText(selectionBadgeText(m_selection_state.kind));
    m_editor_widget->setVisible(false);
    m_has_loaded_request = false;
    clearValidationErrors();

    if (m_selection_state.kind == SelectionKind::Layer
        && m_selection_state.layer_index >= 0
        && m_selection_state.layer_index < m_document_data.layers.size())
    {
        const LayerData &layer = m_document_data.layers.at(m_selection_state.layer_index);
        m_title_label->setText(layer.display_name);
        m_meta_label->setText(QStringLiteral("File layer / imported source"));
        m_geometry_label->setText(QStringLiteral("Summary"));
        m_geometry_body_label->setText(layerSummaryText(layer));
        m_hint_label->setText(QStringLiteral("Select a primitive to edit its in-memory style and coordinates."));
        return;
    }

    if (m_selection_state.kind == SelectionKind::Primitive
        && m_selection_state.layer_index >= 0
        && m_selection_state.layer_index < m_document_data.layers.size())
    {
        const LayerData &layer = m_document_data.layers.at(m_selection_state.layer_index);
        if (m_selection_state.primitive_index >= 0 && m_selection_state.primitive_index < layer.primitives.size())
        {
            const LayerPrimitiveData &primitive = layer.primitives.at(m_selection_state.primitive_index);
            m_title_label->setText(primitive.display_name);
            m_meta_label->setText(QStringLiteral("Primitive / selected in %1").arg(layer.display_name));
            m_geometry_label->setText(QStringLiteral("Geometry"));
            m_geometry_body_label->setText(primitiveGeometryText(layer, m_selection_state.primitive_index));
            m_hint_label->setText(
                QStringLiteral("Apply updates the current in-memory primitive. Reset discards unsaved edits."));
            m_editor_widget->setVisible(true);
            loadPrimitiveEditor(layer, m_selection_state.primitive_index);
            return;
        }
    }

    m_title_label->setText(QStringLiteral("No selection"));
    m_meta_label->setText(QStringLiteral("Select a layer or primitive to inspect its details."));
    m_geometry_label->setText(QStringLiteral("Geometry"));
    m_geometry_body_label->setText(QStringLiteral("Nothing selected."));
    m_hint_label->setText(QStringLiteral("Primitive editing becomes available after you select one shape."));
}

void InspectorPanel::loadPrimitiveEditor(const LayerData &layer, int primitiveIndex)
{
    PrimitiveEditRequest request;
    if (!buildPrimitiveEditRequest(layer, primitiveIndex, &request))
    {
        return;
    }

    m_is_loading_form = true;
    request.layer_index = m_selection_state.layer_index;
    request.primitive_index = primitiveIndex;
    m_loaded_request = request;
    m_has_loaded_request = true;

    m_stroke_color_line_edit->setText(request.stroke_color_text);
    m_fill_enabled_check_box->setChecked(request.fill_enabled);
    m_fill_color_line_edit->setText(request.fill_color_text);
    m_width_line_edit->setText(request.width_text);
    m_point_size_line_edit->setText(request.point_size_text);
    m_coordinates_text_edit->setPlainText(request.coordinates_text);
    updateVisibleEditorFields(request.primitive_kind);
    m_is_loading_form = false;

    clearValidationErrors();
    updateEditorState();
}

void InspectorPanel::reloadCurrentPrimitiveEditor()
{
    if (m_selection_state.kind != SelectionKind::Primitive || m_selection_state.layer_index < 0
        || m_selection_state.layer_index >= m_document_data.layers.size())
    {
        return;
    }

    const LayerData &layer = m_document_data.layers.at(m_selection_state.layer_index);
    if (m_selection_state.primitive_index < 0 || m_selection_state.primitive_index >= layer.primitives.size())
    {
        return;
    }

    loadPrimitiveEditor(layer, m_selection_state.primitive_index);
}

PrimitiveEditRequest InspectorPanel::currentEditRequest() const
{
    PrimitiveEditRequest request = m_loaded_request;
    request.layer_index = m_selection_state.layer_index;
    request.primitive_index = m_selection_state.primitive_index;
    request.stroke_color_text = m_stroke_color_line_edit->text().trimmed();
    request.fill_enabled = m_fill_enabled_check_box->isChecked();
    request.fill_color_text = m_fill_color_line_edit->text().trimmed();
    request.width_text = m_width_line_edit->text().trimmed();
    request.point_size_text = m_point_size_line_edit->text().trimmed();
    request.coordinates_text = m_coordinates_text_edit->toPlainText();
    return request;
}

void InspectorPanel::updateEditorState()
{
    if (!m_has_loaded_request)
    {
        m_apply_button->setEnabled(false);
        m_reset_button->setEnabled(false);
        m_edit_state_label->setText(QStringLiteral("Select a primitive to edit it."));
        return;
    }

    const bool isDirty = currentEditRequest() != m_loaded_request;
    m_apply_button->setEnabled(isDirty);
    m_reset_button->setEnabled(isDirty);
    m_edit_state_label->setText(
        isDirty ? QStringLiteral("Unsaved changes pending.") : QStringLiteral("No pending changes."));
}

void InspectorPanel::setFieldError(QWidget *editor, QLabel *errorLabel, const QString &message)
{
    if (editor != nullptr)
    {
        editor->setProperty("validationState", message.isEmpty() ? QString() : QStringLiteral("error"));
        refreshValidationStyle(editor);
    }

    if (errorLabel != nullptr)
    {
        errorLabel->setText(message);
        errorLabel->setVisible(!message.isEmpty());
    }
}

void InspectorPanel::updateVisibleEditorFields(PrimitiveKind kind)
{
    const bool showFill = kind == PrimitiveKind::Polygon;
    const bool showWidth = kind != PrimitiveKind::Point;

    m_fill_section_widget->setVisible(showFill);
    m_width_section_widget->setVisible(showWidth);
    m_fill_color_line_edit->setEnabled(showFill && m_fill_enabled_check_box->isChecked());
}

} // namespace PolyShow
