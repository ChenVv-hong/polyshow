#include "ui/InspectorPanel.h"

#include "core/PrimitiveEditing.h"
#include "ui/ColorField.h"
#include "ui/EditorPanelHeader.h"
#include "ui/InspectorSection.h"
#include "ui/MaterialIconLabel.h"

#include <QCheckBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QPlainTextEdit>
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
QString primitiveKindText(PrimitiveKind kind, int vertexCount)
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

struct LayerSummary
{
    int primitive_count {0};
    int visible_count {0};
    int hidden_count {0};
    int polygon_count {0};
    int polyline_count {0};
    int point_count {0};
};

/// Counts one layer's primitive summary values.
LayerSummary layerSummary(const LayerData &layer)
{
    LayerSummary summary;
    summary.primitive_count = layer.primitives.size();

    for (const LayerPrimitiveData &primitive : layer.primitives)
    {
        if (primitive.visible)
        {
            ++summary.visible_count;
        }

        switch (primitive.reference.kind)
        {
        case PrimitiveKind::Point:
            ++summary.point_count;
            break;
        case PrimitiveKind::Polyline:
            ++summary.polyline_count;
            break;
        case PrimitiveKind::Polygon:
            ++summary.polygon_count;
            break;
        }
    }

    summary.hidden_count = summary.primitive_count - summary.visible_count;
    return summary;
}

QString layerIconName(const LayerData &layer)
{
    switch (layer.layer_type)
    {
    case LayerType::InternalIpc:
        return QStringLiteral("settings_input_component");
    case LayerType::ExternalFileNormal:
    case LayerType::InternalNormal:
    default:
        return QStringLiteral("folder");
    }
}

QString layerTypeText(const LayerData &layer)
{
    switch (layer.layer_type)
    {
    case LayerType::ExternalFileNormal:
        return QStringLiteral("File layer");
    case LayerType::InternalNormal:
        return QStringLiteral("Internal layer");
    case LayerType::InternalIpc:
        return QStringLiteral("IPC layer");
    default:
        return QStringLiteral("Layer");
    }
}

QString primitiveIconName(PrimitiveKind kind, int vertexCount)
{
    switch (kind)
    {
    case PrimitiveKind::Point:
        return QStringLiteral("radio_button_checked");
    case PrimitiveKind::Polyline:
        return vertexCount == 2 ? QStringLiteral("show_chart") : QStringLiteral("timeline");
    case PrimitiveKind::Polygon:
        return QStringLiteral("pentagon");
    default:
        return QStringLiteral("deployed_code");
    }
}

/// Creates a section label for the inspector layout.
QLabel *createSectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("role", QStringLiteral("sectionTitle"));
    label->setObjectName(QStringLiteral("controlLabel"));
    return label;
}

/// Deletes all child widgets/items from one layout.
void clearLayout(QLayout *layout)
{
    if (layout == nullptr)
    {
        return;
    }

    while (QLayoutItem *item = layout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }
}

/// Creates one compact read-only field row matching the web inspector.
QWidget *createReadonlyFieldRow(
    QWidget *parent,
    const QString &iconName,
    const QString &labelText,
    const QString &valueText)
{
    auto *row = new QWidget(parent);
    row->setObjectName(QStringLiteral("inspectorFieldRow"));
    row->setFixedHeight(23);

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(6, 0, 6, 0);
    layout->setSpacing(6);

    auto *icon = new MaterialIconLabel(iconName, row);
    icon->setProperty("iconRole", QStringLiteral("field"));
    icon->setIconPixelSize(16);
    layout->addWidget(icon);

    auto *label = new QLabel(labelText, row);
    label->setObjectName(QStringLiteral("inspectorFieldLabel"));
    label->setFixedWidth(78);
    label->setTextFormat(Qt::PlainText);
    layout->addWidget(label);

    auto *value = new QLabel(valueText, row);
    value->setObjectName(QStringLiteral("inspectorReadonly"));
    value->setTextFormat(Qt::PlainText);
    layout->addWidget(value, 1);

    return row;
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
    QWidget *parent, const QString &title, QWidget *editor, QLabel **errorLabel = nullptr)
{
    auto *section = new QWidget(parent);
    section->setObjectName(QStringLiteral("fieldSection"));
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(6, 5, 6, 6);
    layout->setSpacing(4);

    layout->addWidget(createSectionTitle(title, section));
    layout->addWidget(editor);

    if (errorLabel != nullptr)
    {
        QLabel *fieldErrorLabel = createErrorLabel(section);
        layout->addWidget(fieldErrorLabel);
        *errorLabel = fieldErrorLabel;
    }

    return section;
}

} // namespace

InspectorPanel::InspectorPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("inspector"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new EditorPanelHeader(QStringLiteral("tune"), QStringLiteral("Properties"), this);
    m_badge_label = new QLabel(header);
    m_badge_label->setObjectName(QStringLiteral("inspectorBadge"));
    header->actionsLayout()->addWidget(m_badge_label);
    layout->addWidget(header);

    auto *objectRow = new QWidget(this);
    objectRow->setObjectName(QStringLiteral("inspectorObjectRow"));
    objectRow->setAttribute(Qt::WA_StyledBackground, true);
    objectRow->setFixedHeight(34);
    auto *objectLayout = new QHBoxLayout(objectRow);
    objectLayout->setContentsMargins(10, 0, 10, 0);
    objectLayout->setSpacing(7);

    m_object_icon_label = new MaterialIconLabel(QStringLiteral("deployed_code"), objectRow);
    objectLayout->addWidget(m_object_icon_label);

    m_title_label = new QLabel(objectRow);
    m_title_label->setObjectName(QStringLiteral("inspectorObjectTitle"));
    objectLayout->addWidget(m_title_label, 1);
    layout->addWidget(objectRow);

    auto *body = new QWidget(this);
    body->setObjectName(QStringLiteral("inspectorBody"));
    body->setAttribute(Qt::WA_StyledBackground, true);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(6, 6, 6, 6);
    bodyLayout->setSpacing(6);

    m_geometry_section = new InspectorSection(QStringLiteral("Geometry"), body);
    bodyLayout->addWidget(m_geometry_section);
    m_geometry_content_layout = m_geometry_section->contentLayout();

    m_style_section = new InspectorSection(QStringLiteral("Style"), body);
    bodyLayout->addWidget(m_style_section);

    m_editor_widget = new QWidget(m_style_section);
    m_editor_widget->setObjectName(QStringLiteral("inspectorEditor"));
    auto *editorLayout = new QVBoxLayout(m_editor_widget);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(6);

    m_stroke_color_field = new ColorField(m_editor_widget);
    m_stroke_color_field->setPlaceholderText(QStringLiteral("#RRGGBB or #RRGGBBAA"));
    editorLayout->addWidget(createFieldSection(m_editor_widget, QStringLiteral("Stroke Color"), m_stroke_color_field));

    auto *fillWrapper = new QWidget(m_editor_widget);
    fillWrapper->setObjectName(QStringLiteral("fieldSection"));
    auto *fillLayout = new QVBoxLayout(fillWrapper);
    fillLayout->setContentsMargins(6, 5, 6, 6);
    fillLayout->setSpacing(6);

    m_fill_enabled_check_box = new QCheckBox(QStringLiteral("Enable Fill"), fillWrapper);
    fillLayout->addWidget(m_fill_enabled_check_box);

    m_fill_color_field = new ColorField(fillWrapper);
    m_fill_color_field->setPlaceholderText(QStringLiteral("#RRGGBB or #RRGGBBAA"));
    fillLayout->addWidget(createFieldSection(fillWrapper, QStringLiteral("Fill Color"), m_fill_color_field));

    m_fill_section_widget = fillWrapper;
    editorLayout->addWidget(m_fill_section_widget);

    m_width_line_edit = new QLineEdit(m_editor_widget);
    m_width_line_edit->setPlaceholderText(QStringLiteral("Greater than 0"));
    m_width_section_widget = createFieldSection(
        m_editor_widget, QStringLiteral("Line Width"), m_width_line_edit, &m_width_error_label);
    editorLayout->addWidget(m_width_section_widget);

    m_point_size_line_edit = new QLineEdit(m_editor_widget);
    m_point_size_line_edit->setPlaceholderText(QStringLiteral("Greater than 0"));
    editorLayout->addWidget(
        createFieldSection(m_editor_widget, QStringLiteral("Point Size"), m_point_size_line_edit, &m_point_size_error_label));

    m_style_section->contentLayout()->addWidget(m_editor_widget);

    m_coordinates_section = new InspectorSection(QStringLiteral("Coordinates"), body);
    bodyLayout->addWidget(m_coordinates_section);

    m_coordinates_text_edit = new QPlainTextEdit(m_coordinates_section);
    m_coordinates_text_edit->setObjectName(QStringLiteral("coordinatesEditor"));
    m_coordinates_text_edit->setPlaceholderText(QStringLiteral("x y"));
    m_coordinates_text_edit->setMinimumHeight(72);
    m_coordinates_text_edit->setProperty("role", QStringLiteral("mono"));
    m_coordinates_text_edit->setFont(QFont(QStringLiteral("IBM Plex Mono"), 10));
    m_coordinates_section->contentLayout()->addWidget(m_coordinates_text_edit);
    m_coordinates_error_label = createErrorLabel(m_coordinates_section);
    m_coordinates_section->contentLayout()->addWidget(m_coordinates_error_label);

    m_coordinates_hint_label = new QLabel(QStringLiteral("Invalid coordinates hide preview."), m_coordinates_section);
    m_coordinates_hint_label->setObjectName(QStringLiteral("inspectorHint"));
    m_coordinates_hint_label->setWordWrap(true);
    m_coordinates_section->contentLayout()->addWidget(m_coordinates_hint_label);

    bodyLayout->addStretch();
    layout->addWidget(body, 1);

    connect(m_stroke_color_field, &ColorField::colorTextCommitted, this, [this](const QString &) {
        if (!m_is_loading_form)
        {
            emit styleChangeRequested(buildStyleChangeRequest(PrimitiveStyleField::StrokeColor));
        }
    });
    connect(m_fill_color_field, &ColorField::colorTextCommitted, this, [this](const QString &) {
        if (!m_is_loading_form)
        {
            emit styleChangeRequested(buildStyleChangeRequest(PrimitiveStyleField::FillColor));
        }
    });
    connect(m_fill_enabled_check_box, &QCheckBox::toggled, this, [this](bool) {
        if (!m_is_loading_form)
        {
            updateVisibleEditorFields(currentPrimitiveKind());
            emit styleChangeRequested(buildStyleChangeRequest(PrimitiveStyleField::FillEnabled));
        }
    });
    connect(m_width_line_edit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_is_loading_form)
        {
            emit styleChangeRequested(buildStyleChangeRequest(PrimitiveStyleField::Width));
        }
    });
    connect(m_point_size_line_edit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_is_loading_form)
        {
            emit styleChangeRequested(buildStyleChangeRequest(PrimitiveStyleField::PointSize));
        }
    });
    connect(m_coordinates_text_edit, &QPlainTextEdit::textChanged, this, [this]() {
        if (!m_is_loading_form)
        {
            emit coordinateDraftChanged(currentCoordinateDraft());
        }
    });

    updateContent();
}

void InspectorPanel::loadSelectionContext(const DocumentData &documentData, const SelectionState &selectionState)
{
    m_document_data = documentData;
    m_selection_state = selectionState;
    updateContent();
}

void InspectorPanel::setStyleFieldError(PrimitiveStyleField field, const QString &message)
{
    switch (field)
    {
    case PrimitiveStyleField::StrokeColor:
        m_validation_errors.stroke_color = message;
        break;
    case PrimitiveStyleField::FillColor:
        m_validation_errors.fill_color = message;
        break;
    case PrimitiveStyleField::Width:
        m_validation_errors.width = message;
        break;
    case PrimitiveStyleField::PointSize:
        m_validation_errors.point_size = message;
        break;
    case PrimitiveStyleField::FillEnabled:
        break;
    }

    updateFieldErrors();
}

void InspectorPanel::clearStyleFieldError(PrimitiveStyleField field)
{
    setStyleFieldError(field, QString());
}

void InspectorPanel::setCoordinateError(const QString &message)
{
    m_validation_errors.coordinates = message;
    updateFieldErrors();
}

void InspectorPanel::clearCoordinateError()
{
    setCoordinateError(QString());
}

void InspectorPanel::updateContent()
{
    m_validation_errors.clear();
    m_badge_label->setText(selectionBadgeText(m_selection_state.kind));
    m_editor_widget->setVisible(false);
    m_style_section->setVisible(false);
    m_coordinates_section->setVisible(false);
    clearLayout(m_geometry_content_layout);

    if (m_selection_state.kind == SelectionKind::Layer
        && m_selection_state.layer_index >= 0
        && m_selection_state.layer_index < m_document_data.layers.size())
    {
        const LayerData &layer = m_document_data.layers.at(m_selection_state.layer_index);
        const LayerSummary summary = layerSummary(layer);
        m_object_icon_label->setIconName(layerIconName(layer));
        m_title_label->setText(layer.display_name);
        m_geometry_section->setTitle(QStringLiteral("Summary"));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section, QStringLiteral("folder"), QStringLiteral("Type"), layerTypeText(layer)));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section, QStringLiteral("category"), QStringLiteral("Primitives"), QString::number(summary.primitive_count)));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section, QStringLiteral("visibility"), QStringLiteral("Visible"), QString::number(summary.visible_count)));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section, QStringLiteral("visibility_off"), QStringLiteral("Hidden"), QString::number(summary.hidden_count)));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section, QStringLiteral("pentagon"), QStringLiteral("Polygons"), QString::number(summary.polygon_count)));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section, QStringLiteral("timeline"), QStringLiteral("Polylines"), QString::number(summary.polyline_count)));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section, QStringLiteral("radio_button_checked"), QStringLiteral("Points"), QString::number(summary.point_count)));
        updateFieldErrors();
        return;
    }

    if (hasActivePrimitiveSelection())
    {
        const LayerData &layer = m_document_data.layers.at(m_selection_state.layer_index);
        const LayerPrimitiveData &primitive = layer.primitives.at(m_selection_state.primitive_index);
        const QVector<Point2D> points = primitivePoints(layer, m_selection_state.primitive_index);
        const QRectF bounds = pointsBounds(points);
        m_object_icon_label->setIconName(primitiveIconName(primitive.reference.kind, points.size()));
        m_title_label->setText(primitive.display_name);
        m_geometry_section->setTitle(QStringLiteral("Geometry"));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section,
            primitiveIconName(primitive.reference.kind, points.size()),
            QStringLiteral("Type"),
            primitiveKindText(primitive.reference.kind, points.size())));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section, QStringLiteral("hub"), QStringLiteral("Vertices"), QString::number(points.size())));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section,
            QStringLiteral("select_all"),
            QStringLiteral("Bounds"),
            QStringLiteral("%1 x %2").arg(bounds.width(), 0, 'f', 2).arg(bounds.height(), 0, 'f', 2)));
        m_geometry_content_layout->addWidget(createReadonlyFieldRow(
            m_geometry_section,
            QStringLiteral("visibility"),
            QStringLiteral("Visible"),
            primitive.visible ? QStringLiteral("true") : QStringLiteral("false")));
        m_editor_widget->setVisible(true);
        m_style_section->setVisible(true);
        m_coordinates_section->setVisible(true);
        updateVisibleEditorFields(primitive.reference.kind);
        loadPrimitiveEditor(layer, m_selection_state.primitive_index);
        updateFieldErrors();
        return;
    }

    m_object_icon_label->setIconName(QStringLiteral("info"));
    m_title_label->setText(QStringLiteral("No selection"));
    m_geometry_section->setTitle(QStringLiteral("Geometry"));
    m_geometry_content_layout->addWidget(createReadonlyFieldRow(
        m_geometry_section, QStringLiteral("info"), QStringLiteral("State"), QStringLiteral("Nothing selected")));
    updateFieldErrors();
}

void InspectorPanel::loadPrimitiveEditor(const LayerData &layer, int primitiveIndex)
{
    PrimitiveEditValues values;
    if (!readPrimitiveEditValues(layer, primitiveIndex, &values))
    {
        return;
    }

    m_is_loading_form = true;
    m_stroke_color_field->setColorText(formatColorText(values.style.color));
    m_fill_enabled_check_box->setChecked(values.style.fill_enabled);
    m_fill_color_field->setColorText(formatColorText(values.style.fill_color));
    m_width_line_edit->setText(QString::number(values.style.width, 'f', 2));
    m_point_size_line_edit->setText(QString::number(values.style.point_size, 'f', 2));
    m_coordinates_text_edit->setPlainText(formatCoordinateText(values.points));
    updateVisibleEditorFields(layer.primitives.at(primitiveIndex).reference.kind);
    m_is_loading_form = false;
}

void InspectorPanel::setTextFieldError(QWidget *editor, QLabel *errorLabel, const QString &message)
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

void InspectorPanel::updateFieldErrors()
{
    m_stroke_color_field->setErrorMessage(m_validation_errors.stroke_color);
    m_fill_color_field->setErrorMessage(m_validation_errors.fill_color);
    setTextFieldError(m_width_line_edit, m_width_error_label, m_validation_errors.width);
    setTextFieldError(m_point_size_line_edit, m_point_size_error_label, m_validation_errors.point_size);
    setTextFieldError(m_coordinates_text_edit, m_coordinates_error_label, m_validation_errors.coordinates);
}

void InspectorPanel::updateVisibleEditorFields(PrimitiveKind kind)
{
    const bool showFill = kind == PrimitiveKind::Polygon;
    const bool showWidth = kind != PrimitiveKind::Point;

    m_fill_section_widget->setVisible(showFill);
    m_width_section_widget->setVisible(showWidth);
    m_fill_color_field->setEnabled(showFill && m_fill_enabled_check_box->isChecked());
}

bool InspectorPanel::hasActivePrimitiveSelection() const
{
    return m_selection_state.kind == SelectionKind::Primitive
        && m_selection_state.layer_index >= 0
        && m_selection_state.layer_index < m_document_data.layers.size()
        && m_selection_state.primitive_index >= 0
        && m_selection_state.primitive_index < m_document_data.layers.at(m_selection_state.layer_index).primitives.size();
}

PrimitiveKind InspectorPanel::currentPrimitiveKind() const
{
    if (!hasActivePrimitiveSelection())
    {
        return PrimitiveKind::Point;
    }

    return m_document_data.layers.at(m_selection_state.layer_index)
        .primitives.at(m_selection_state.primitive_index)
        .reference.kind;
}

PrimitiveStyleChangeRequest InspectorPanel::buildStyleChangeRequest(PrimitiveStyleField field) const
{
    PrimitiveStyleChangeRequest request;
    request.field = field;
    request.primitive_kind = currentPrimitiveKind();
    request.layer_index = m_selection_state.layer_index;
    request.primitive_index = m_selection_state.primitive_index;

    switch (field)
    {
    case PrimitiveStyleField::StrokeColor:
        request.text_value = m_stroke_color_field->colorText();
        break;
    case PrimitiveStyleField::FillColor:
        request.text_value = m_fill_color_field->colorText();
        break;
    case PrimitiveStyleField::Width:
        request.text_value = m_width_line_edit->text().trimmed();
        break;
    case PrimitiveStyleField::PointSize:
        request.text_value = m_point_size_line_edit->text().trimmed();
        break;
    case PrimitiveStyleField::FillEnabled:
        request.bool_value = m_fill_enabled_check_box->isChecked();
        break;
    }

    return request;
}

PrimitiveCoordinateDraft InspectorPanel::currentCoordinateDraft() const
{
    PrimitiveCoordinateDraft draft;
    draft.primitive_kind = currentPrimitiveKind();
    draft.layer_index = m_selection_state.layer_index;
    draft.primitive_index = m_selection_state.primitive_index;
    draft.coordinates_text = m_coordinates_text_edit->toPlainText();
    return draft;
}

} // namespace PolyShow
