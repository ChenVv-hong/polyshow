#include "ui/InspectorPanel.h"

#include "ui/PanelFrame.h"

#include <algorithm>
#include <QLabel>
#include <QRectF>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace PolyShow
{

namespace
{

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

QString formatColor(const QColor &color)
{
    return color.name(QColor::HexArgb).toUpper();
}

QVector<Point2D> primitivePoints(const LayerData &layer, const LayerPrimitiveData &primitive)
{
    switch (primitive.reference.kind)
    {
    case PrimitiveKind::Point:
        return QVector<Point2D> {layer.geometry.points.at(primitive.reference.index).point};
    case PrimitiveKind::Polyline:
        return layer.geometry.polylines.at(primitive.reference.index).vertices;
    case PrimitiveKind::Polygon:
        return layer.geometry.polygons.at(primitive.reference.index).vertices;
    default:
        return {};
    }
}

PrimitiveStyle primitiveStyle(const LayerData &layer, const LayerPrimitiveData &primitive)
{
    switch (primitive.reference.kind)
    {
    case PrimitiveKind::Point:
        return layer.geometry.points.at(primitive.reference.index).style;
    case PrimitiveKind::Polyline:
        return layer.geometry.polylines.at(primitive.reference.index).style;
    case PrimitiveKind::Polygon:
        return layer.geometry.polygons.at(primitive.reference.index).style;
    default:
        return {};
    }
}

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

QString primitiveGeometryText(const LayerData &layer, const LayerPrimitiveData &primitive)
{
    const QVector<Point2D> points = primitivePoints(layer, primitive);
    const QRectF bounds = pointsBounds(points);

    return QStringLiteral("Type        %1\nVertices    %2\nBounds      %3 x %4")
        .arg(primitiveKindText(primitive.reference.kind))
        .arg(points.size())
        .arg(bounds.width(), 0, 'f', 1)
        .arg(bounds.height(), 0, 'f', 1);
}

QString primitiveCoordinatesText(const LayerData &layer, const LayerPrimitiveData &primitive)
{
    const QVector<Point2D> points = primitivePoints(layer, primitive);
    QStringList lines;
    const int limit = std::min(static_cast<int>(points.size()), 8);

    for (int index = 0; index < limit; ++index)
    {
        lines.append(QStringLiteral("P%1   (%2, %3)")
                         .arg(index + 1)
                         .arg(points.at(index).x, 0, 'f', 1)
                         .arg(points.at(index).y, 0, 'f', 1));
    }

    if (points.size() > limit)
    {
        lines.append(QStringLiteral("..."));
    }

    return lines.join(QStringLiteral("\n"));
}

QString primitiveStyleText(const LayerData &layer, const LayerPrimitiveData &primitive)
{
    const PrimitiveStyle style = primitiveStyle(layer, primitive);
    return QStringLiteral("Stroke      %1\nFill        %2\nLine Width  %3 px\nVisible     %4")
        .arg(formatColor(style.color))
        .arg(formatColor(style.fill_color))
        .arg(style.width, 0, 'f', 1)
        .arg(primitive.visible ? QStringLiteral("true") : QStringLiteral("false"));
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

    m_geometry_label = new QLabel(QStringLiteral("Geometry"), card);
    m_geometry_label->setProperty("role", QStringLiteral("sectionTitle"));
    cardLayout->addWidget(m_geometry_label);

    m_geometry_body_label = new QLabel(card);
    m_geometry_body_label->setProperty("role", QStringLiteral("mono"));
    m_geometry_body_label->setTextFormat(Qt::PlainText);
    m_geometry_body_label->setWordWrap(true);
    cardLayout->addWidget(m_geometry_body_label);

    m_coordinates_label = new QLabel(QStringLiteral("Coordinates"), card);
    m_coordinates_label->setProperty("role", QStringLiteral("sectionTitle"));
    cardLayout->addWidget(m_coordinates_label);

    m_coordinates_body_label = new QLabel(card);
    m_coordinates_body_label->setProperty("role", QStringLiteral("mono"));
    m_coordinates_body_label->setTextFormat(Qt::PlainText);
    m_coordinates_body_label->setWordWrap(true);
    cardLayout->addWidget(m_coordinates_body_label);

    m_style_label = new QLabel(QStringLiteral("Style"), card);
    m_style_label->setProperty("role", QStringLiteral("sectionTitle"));
    cardLayout->addWidget(m_style_label);

    m_style_body_label = new QLabel(card);
    m_style_body_label->setProperty("role", QStringLiteral("mono"));
    m_style_body_label->setTextFormat(Qt::PlainText);
    m_style_body_label->setWordWrap(true);
    cardLayout->addWidget(m_style_body_label);

    m_hint_label = new QLabel(card);
    m_hint_label->setWordWrap(true);
    cardLayout->addWidget(m_hint_label);

    cardLayout->addStretch();
    layout->addWidget(card);

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

void InspectorPanel::updateContent()
{
    m_badge_label->setText(selectionBadgeText(m_selection_state.kind));

    if (m_selection_state.kind == SelectionKind::Layer
        && m_selection_state.layer_index >= 0
        && m_selection_state.layer_index < m_document_data.layers.size())
    {
        const LayerData &layer = m_document_data.layers.at(m_selection_state.layer_index);
        m_title_label->setText(layer.display_name);
        m_meta_label->setText(QStringLiteral("File layer / imported source"));
        m_geometry_label->setText(QStringLiteral("Summary"));
        m_geometry_body_label->setText(layerSummaryText(layer));
        m_coordinates_label->setText(QStringLiteral("Primitive Counts"));
        m_coordinates_body_label->setText(
            QStringLiteral("Polygon   %1\nPolyline  %2\nPoint     %3")
                .arg(layer.geometry.polygons.size())
                .arg(layer.geometry.polylines.size())
                .arg(layer.geometry.points.size()));
        m_style_label->setText(QStringLiteral("Source"));
        m_style_body_label->setText(
            QStringLiteral("File Name  %1\nLayer Name %2\nPath      %3")
                .arg(layer.display_name)
                .arg(layer.display_name)
                .arg(layer.file_path));
        m_hint_label->setText(QStringLiteral("Future editing: rename layer, toggle visibility, bulk style updates"));
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
            m_geometry_body_label->setText(primitiveGeometryText(layer, primitive));
            m_coordinates_label->setText(QStringLiteral("Coordinates"));
            m_coordinates_body_label->setText(primitiveCoordinatesText(layer, primitive));
            m_style_label->setText(QStringLiteral("Style"));
            m_style_body_label->setText(primitiveStyleText(layer, primitive));
            m_hint_label->setText(
                QStringLiteral("Future editing: rename, coordinates, stroke, fill, visibility"));
            return;
        }
    }

    m_title_label->setText(QStringLiteral("No selection"));
    m_meta_label->setText(QStringLiteral("Select a layer or primitive to inspect its details."));
    m_geometry_label->setText(QStringLiteral("Geometry"));
    m_geometry_body_label->setText(QStringLiteral("Nothing selected."));
    m_coordinates_label->setText(QStringLiteral("Coordinates"));
    m_coordinates_body_label->setText(QStringLiteral("-"));
    m_style_label->setText(QStringLiteral("Style"));
    m_style_body_label->setText(QStringLiteral("-"));
    m_hint_label->setText(QStringLiteral("The inspector is read-only in this iteration."));
}

} // namespace PolyShow
