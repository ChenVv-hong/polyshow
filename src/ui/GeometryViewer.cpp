#include "ui/GeometryViewer.h"

#include "core/GeometryScene.h"
#include "style/RenderTheme.h"

#include <QCursor>
#include <QFontMetrics>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSvgRenderer>
#include <QVector>
#include <QWheelEvent>

#include <cmath>

namespace PolyShow
{

namespace
{

constexpr qreal kScaleBarTargetPixels = 120.0;
constexpr qreal kTargetMinorGridPixels = 48.0;
constexpr int kMajorGridFactor = 5;
constexpr int kScaleBarMargin = 16;
constexpr int kScaleBarPadding = 8;
constexpr int kScaleBarTickHeight = 10;
constexpr int kHintCardMargin = 16;
constexpr int kHintCardPadding = 10;
constexpr int kHintCardCornerRadius = 8;
constexpr int kHintCardMaxWidth = 220;
constexpr int kHintCardMinimumWidth = 112;
constexpr int kHintCardGap = 12;
constexpr int kHintCardIconSize = 24;
constexpr int kHintCardIconTextGap = 8;
constexpr int kHintCardRowGap = 6;
constexpr qreal kFallbackFitHalfExtent = 250.0;
constexpr int kLayerIndexRole = 1;
constexpr int kPrimitiveIndexRole = 2;
constexpr int kSelectionOverlayRole = 3;

enum class HintIconKind
{
    WheelZoom,
    MiddleDrag,
    LeftClick,
    RightClick
};

struct HintRow
{
    HintIconKind icon_kind {HintIconKind::LeftClick};
    QString action;
};

/// Rounds a raw scale bar length down to a human-friendly 1/2/5 * 10^n increment.
qreal niceScaleLength(qreal rawLength)
{
    if (rawLength <= 0.0)
    {
        return 1.0;
    }

    const qreal exponent = std::floor(std::log10(rawLength));
    const qreal power = std::pow(10.0, exponent);
    const qreal fraction = rawLength / power;

    qreal niceFraction = 1.0;
    if (fraction >= 5.0)
    {
        niceFraction = 5.0;
    }
    else if (fraction >= 2.0)
    {
        niceFraction = 2.0;
    }

    return niceFraction * power;
}

/// Rounds a raw grid spacing up to a human-friendly 1/2/5 * 10^n increment.
qreal niceGridStep(qreal rawStep)
{
    if (rawStep <= 0.0)
    {
        return 1.0;
    }

    const qreal exponent = std::floor(std::log10(rawStep));
    const qreal power = std::pow(10.0, exponent);
    const qreal fraction = rawStep / power;

    qreal niceFraction = 1.0;
    if (fraction > 5.0)
    {
        niceFraction = 10.0;
    }
    else if (fraction > 2.0)
    {
        niceFraction = 5.0;
    }
    else if (fraction > 1.0)
    {
        niceFraction = 2.0;
    }

    return niceFraction * power;
}

/// Formats one scale label using compact numeric text.
QString scaleLabelText(qreal sceneLength)
{
    const qreal roundedValue = std::round(sceneLength);
    if (std::abs(sceneLength - roundedValue) < 1e-6)
    {
        return QString::number(static_cast<qint64>(roundedValue));
    }

    return QString::number(sceneLength, 'g', 3);
}

QString hintIconResourcePath(HintIconKind iconKind, ThemeMode themeMode)
{
    switch (iconKind)
    {
    case HintIconKind::WheelZoom:
        return themeMode == ThemeMode::Dark ? QStringLiteral(":/overlay/mouse_wheel_zoom_dark.svg")
                                            : QStringLiteral(":/overlay/mouse_wheel_zoom_light.svg");
    case HintIconKind::MiddleDrag:
        return themeMode == ThemeMode::Dark ? QStringLiteral(":/overlay/mouse_middle_drag_dark.svg")
                                            : QStringLiteral(":/overlay/mouse_middle_drag_light.svg");
    case HintIconKind::LeftClick:
        return themeMode == ThemeMode::Dark ? QStringLiteral(":/overlay/mouse_left_click_dark.svg")
                                            : QStringLiteral(":/overlay/mouse_left_click_light.svg");
    case HintIconKind::RightClick:
    default:
        return themeMode == ThemeMode::Dark ? QStringLiteral(":/overlay/mouse_right_click_dark.svg")
                                            : QStringLiteral(":/overlay/mouse_right_click_light.svg");
    }
}

QVector<HintRow> hintRows(GeometryViewer::ToolMode toolMode)
{
    QVector<HintRow> rows {
        HintRow {HintIconKind::WheelZoom, QStringLiteral("Zoom")},
        HintRow {HintIconKind::MiddleDrag, QStringLiteral("Pan")}
    };

    switch (toolMode)
    {
    case GeometryViewer::ToolMode::Browse:
        rows.append(HintRow {HintIconKind::LeftClick, QStringLiteral("Select")});
        break;
    case GeometryViewer::ToolMode::DrawPoint:
        rows.append(HintRow {HintIconKind::LeftClick, QStringLiteral("Create point")});
        break;
    case GeometryViewer::ToolMode::DrawPolyline:
        rows.append(HintRow {HintIconKind::LeftClick, QStringLiteral("Add vertex")});
        rows.append(HintRow {HintIconKind::RightClick, QStringLiteral("Finish polyline")});
        break;
    case GeometryViewer::ToolMode::DrawPolygon:
        rows.append(HintRow {HintIconKind::LeftClick, QStringLiteral("Add vertex")});
        rows.append(HintRow {HintIconKind::RightClick, QStringLiteral("Finish polygon")});
        break;
    default:
        break;
    }

    return rows;
}

void drawHintActionRow(
    QPainter *painter,
    const QRect &rowRect,
    const QString &iconPath,
    const QString &actionLabel,
    const QColor &textColor,
    const QFont &textFont)
{
    if (painter == nullptr)
    {
        return;
    }

    const QRect iconRect(rowRect.left(), rowRect.top(), kHintCardIconSize, kHintCardIconSize);
    QSvgRenderer renderer(iconPath);
    renderer.render(painter, iconRect);

    painter->setFont(textFont);
    painter->setPen(textColor);
    painter->drawText(
        QRect(
            iconRect.right() + 1 + kHintCardIconTextGap,
            rowRect.top(),
            rowRect.width() - kHintCardIconSize - kHintCardIconTextGap,
            rowRect.height()),
        Qt::AlignVCenter | Qt::AlignLeft,
        actionLabel);
}

} // namespace

/// Creates the viewer and configures the default interaction behavior.
GeometryViewer::GeometryViewer(QWidget *parent)
    : QGraphicsView(parent)
{
    const RenderColors &renderColors = RenderTheme::colors();

    // Enable smoothing so points and lines remain readable while zooming.
    setRenderHint(QPainter::Antialiasing, true);

    // Keep future pixmap-based content looking acceptable when scaled.
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setDragMode(QGraphicsView::NoDrag);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(renderColors.canvas_background);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
}

void GeometryViewer::setToolMode(ToolMode toolMode)
{
    if (m_tool_mode == toolMode)
    {
        return;
    }

    m_tool_mode = toolMode;
    const QPoint viewPosition = viewport()->mapFromGlobal(QCursor::pos());
    updateInteractionCursor(viewPosition);
    viewport()->update();
}

GeometryViewer::ToolMode GeometryViewer::toolMode() const
{
    return m_tool_mode;
}

/// Applies the standard zoom-in ratio.
void GeometryViewer::zoomIn()
{
    applyZoom(1.15);
}

/// Applies the standard zoom-out ratio.
void GeometryViewer::zoomOut()
{
    applyZoom(1.0 / 1.15);
}

/// Fits all scene items into the current viewport.
void GeometryViewer::fitScene()
{
    if (scene() == nullptr)
    {
        return;
    }

    const QRectF itemsRect = scene()->itemsBoundingRect();
    const QRectF fallbackRect(
        -kFallbackFitHalfExtent,
        -kFallbackFitHalfExtent,
        kFallbackFitHalfExtent * 2.0,
        kFallbackFitHalfExtent * 2.0);
    fitInView(itemsRect.isEmpty() ? fallbackRect : itemsRect, Qt::KeepAspectRatio);
}

/// Resets the transform and recenters the origin.
void GeometryViewer::resetViewTransform()
{
    resetTransform();
    centerOn(0.0, 0.0);
}

/// Handles wheel-based zoom input.
void GeometryViewer::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
    {
        zoomIn();
    }
    else
    {
        zoomOut();
    }

    event->accept();
}

/// Draws a dynamic grid for the currently visible scene region.
void GeometryViewer::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawBackground(painter, rect);

    const auto *geometryScene = qobject_cast<const GeometryScene *>(scene());
    if (geometryScene == nullptr || !geometryScene->isGridVisible())
    {
        return;
    }

    const RenderColors &renderColors = RenderTheme::colors();
    const qreal pixelsPerSceneUnit = std::max(std::abs(transform().m11()), 1e-6);
    const qreal minorStep = niceGridStep(kTargetMinorGridPixels / pixelsPerSceneUnit);

    QPen minorPen(renderColors.grid_line);
    minorPen.setWidthF(0.0);

    QPen majorPen(renderColors.axis_line);
    majorPen.setWidthF(0.0);

    QPen axisPen(renderColors.axis_line);
    axisPen.setWidthF(0.0);

    const qint64 startColumn = static_cast<qint64>(std::floor(rect.left() / minorStep));
    const qint64 endColumn = static_cast<qint64>(std::ceil(rect.right() / minorStep));
    for (qint64 column = startColumn; column <= endColumn; ++column)
    {
        const qreal x = static_cast<qreal>(column) * minorStep;
        const bool isAxis = column == 0;
        const bool isMajor = (column % kMajorGridFactor) == 0;
        painter->setPen(isAxis ? axisPen : (isMajor ? majorPen : minorPen));
        painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
    }

    const qint64 startRow = static_cast<qint64>(std::floor(rect.top() / minorStep));
    const qint64 endRow = static_cast<qint64>(std::ceil(rect.bottom() / minorStep));
    for (qint64 row = startRow; row <= endRow; ++row)
    {
        const qreal y = static_cast<qreal>(row) * minorStep;
        const bool isAxis = row == 0;
        const bool isMajor = (row % kMajorGridFactor) == 0;
        painter->setPen(isAxis ? axisPen : (isMajor ? majorPen : minorPen));
        painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
    }
}

/// Draws a dynamic scale bar overlay anchored to the viewport corner.
void GeometryViewer::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawForeground(painter, rect);

    const qreal pixelsPerSceneUnit = std::max(std::abs(transform().m11()), 1e-6);
    const qreal sceneLength = niceScaleLength(kScaleBarTargetPixels / pixelsPerSceneUnit);
    const int barPixelLength = std::max(1, static_cast<int>(std::round(sceneLength * pixelsPerSceneUnit)));
    const QString labelText = scaleLabelText(sceneLength);

    const QFontMetrics metrics(font());
    const int overlayWidth = std::max(barPixelLength, metrics.horizontalAdvance(labelText)) + kScaleBarPadding * 2;
    const int overlayHeight = metrics.height() + kScaleBarTickHeight + kScaleBarPadding * 3;

    const QRect viewportRect = viewport()->rect();
    const QRect scaleBarRect(
        viewportRect.right() - overlayWidth - kScaleBarMargin + 1,
        viewportRect.bottom() - overlayHeight - kScaleBarMargin + 1,
        overlayWidth,
        overlayHeight);

    QColor backgroundColor = palette().color(QPalette::Window);
    backgroundColor.setAlpha(220);
    const QColor borderColor = palette().color(QPalette::Mid);
    const QColor foregroundColor = palette().color(QPalette::WindowText);

    painter->save();
    painter->resetTransform();
    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(borderColor);
    painter->setBrush(backgroundColor);
    painter->drawRoundedRect(scaleBarRect, 8.0, 8.0);

    painter->setPen(foregroundColor);
    painter->drawText(
        QRect(
            scaleBarRect.left() + kScaleBarPadding,
            scaleBarRect.top() + kScaleBarPadding,
            scaleBarRect.width() - kScaleBarPadding * 2,
            metrics.height()),
        Qt::AlignCenter,
        labelText);

    const int barLeft = scaleBarRect.left() + (scaleBarRect.width() - barPixelLength) / 2;
    const int barRight = barLeft + barPixelLength;
    const int barBaseline = scaleBarRect.bottom() - kScaleBarPadding - 2;
    const int barTickTop = barBaseline - kScaleBarTickHeight;

    QPen scalePen(foregroundColor);
    scalePen.setWidth(2);
    painter->setPen(scalePen);
    painter->drawLine(barLeft, barBaseline, barRight, barBaseline);
    painter->drawLine(barLeft, barTickTop, barLeft, barBaseline);
    painter->drawLine(barRight, barTickTop, barRight, barBaseline);

    painter->restore();

    const int maxHintWidth = std::min(kHintCardMaxWidth, scaleBarRect.left() - kHintCardGap - kHintCardMargin);
    if (maxHintWidth < kHintCardMinimumWidth)
    {
        return;
    }

    const QVector<HintRow> rows = hintRows(m_tool_mode);
    if (rows.isEmpty())
    {
        return;
    }

    const RenderColors &renderColors = RenderTheme::colors();
    const ThemeMode themeMode = RenderTheme::activeTheme();

    QFont bodyFont = font();
    const QFontMetrics bodyMetrics(bodyFont);

    const int actionRowHeight = std::max(bodyMetrics.height(), kHintCardIconSize);
    int maxActionWidth = 0;
    for (const HintRow &row : rows)
    {
        maxActionWidth = std::max(maxActionWidth, bodyMetrics.horizontalAdvance(row.action));
    }

    const int cardWidth = std::min(
        maxHintWidth,
        std::max(kHintCardMinimumWidth, kHintCardPadding * 2 + kHintCardIconSize + kHintCardIconTextGap + maxActionWidth));
    const int contentWidth = cardWidth - kHintCardPadding * 2;
    const int rowCount = rows.size();
    const int rowsHeight = actionRowHeight * rowCount + kHintCardRowGap * std::max(rowCount - 1, 0);
    const int cardHeight = kHintCardPadding * 2 + rowsHeight;

    const QRect hintCardRect(
        kHintCardMargin,
        viewportRect.bottom() - cardHeight - kHintCardMargin + 1,
        cardWidth,
        cardHeight);

    painter->save();
    painter->resetTransform();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(renderColors.overlay_panel_border));
    painter->setBrush(renderColors.overlay_panel_background);
    painter->drawRoundedRect(hintCardRect, kHintCardCornerRadius, kHintCardCornerRadius);

    int cursorY = hintCardRect.top() + kHintCardPadding;
    for (int rowIndex = 0; rowIndex < rows.size(); ++rowIndex)
    {
        const HintRow &row = rows.at(rowIndex);
        const QRect rowRect(
            hintCardRect.left() + kHintCardPadding,
            cursorY,
            contentWidth,
            actionRowHeight);
        drawHintActionRow(
            painter,
            rowRect,
            hintIconResourcePath(row.icon_kind, themeMode),
            row.action,
            renderColors.overlay_body_text,
            bodyFont);
        cursorY = rowRect.bottom() + 1;
        if (rowIndex + 1 < rows.size())
        {
            cursorY += kHintCardRowGap;
        }
    }

    painter->restore();
}

/// Starts panning when the middle mouse button is pressed.
void GeometryViewer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
    {
        m_is_panning = true;
        m_last_mouse_position = event->pos();
        updateDragCursor(true);
        event->accept();
        return;
    }

    if (m_tool_mode != ToolMode::Browse)
    {
        if (event->button() == Qt::LeftButton)
        {
            emit drawingPointRequested(mapToScene(event->pos()));
            updateInteractionCursor(event->pos());
            event->accept();
            return;
        }

        if (event->button() == Qt::RightButton
            && (m_tool_mode == ToolMode::DrawPolyline || m_tool_mode == ToolMode::DrawPolygon))
        {
            emit drawingFinishedRequested();
            updateInteractionCursor(event->pos());
            event->accept();
            return;
        }
    }

    if (event->button() == Qt::LeftButton)
    {
        const QList<QGraphicsItem *> sceneItems = items(event->pos());
        for (QGraphicsItem *item : sceneItems)
        {
            if (item == nullptr || item->data(kSelectionOverlayRole).toBool())
            {
                continue;
            }

            const QVariant layerIndex = item->data(kLayerIndexRole);
            const QVariant primitiveIndex = item->data(kPrimitiveIndexRole);
            if (layerIndex.isValid() && primitiveIndex.isValid())
            {
                emit primitiveActivated(layerIndex.toInt(), primitiveIndex.toInt());
                updateInteractionCursor(event->pos());
                event->accept();
                return;
            }
        }

        emit emptyAreaActivated();
        updateInteractionCursor(event->pos());
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

/// Updates the mouse position signal and pans while middle-dragging.
void GeometryViewer::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF scenePosition = mapToScene(event->pos());
    emit mousePositionChanged(scenePosition);

    if (m_is_panning)
    {
        const QPoint delta = event->pos() - m_last_mouse_position;
        m_last_mouse_position = event->pos();

        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    updateInteractionCursor(event->pos());

    QGraphicsView::mouseMoveEvent(event);
}

/// Stops panning when the middle mouse button is released.
void GeometryViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && m_is_panning)
    {
        m_is_panning = false;
        updateDragCursor(false);
        updateInteractionCursor(event->pos());
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void GeometryViewer::leaveEvent(QEvent *event)
{
    viewport()->unsetCursor();
    emit workspaceHoverExited();
    QGraphicsView::leaveEvent(event);
}

/// Applies a scale transform to the view.
void GeometryViewer::applyZoom(qreal factor)
{
    scale(factor, factor);
    viewport()->update();
}

bool GeometryViewer::hasSelectablePrimitiveAt(const QPoint &viewPosition) const
{
    const QList<QGraphicsItem *> sceneItems = items(viewPosition);
    for (QGraphicsItem *item : sceneItems)
    {
        if (item == nullptr || item->data(kSelectionOverlayRole).toBool())
        {
            continue;
        }

        const QVariant layerIndex = item->data(kLayerIndexRole);
        const QVariant primitiveIndex = item->data(kPrimitiveIndexRole);
        if (layerIndex.isValid() && primitiveIndex.isValid())
        {
            return true;
        }
    }

    return false;
}

void GeometryViewer::updateInteractionCursor(const QPoint &viewPosition)
{
    if (!viewport()->rect().contains(viewPosition))
    {
        viewport()->unsetCursor();
        return;
    }

    if (m_is_panning)
    {
        viewport()->setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (m_tool_mode != ToolMode::Browse)
    {
        viewport()->setCursor(Qt::CrossCursor);
        return;
    }

    viewport()->setCursor(hasSelectablePrimitiveAt(viewPosition) ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

/// Chooses the cursor shape for the current drag state.
void GeometryViewer::updateDragCursor(bool active)
{
    viewport()->setCursor(active ? Qt::ClosedHandCursor : Qt::ArrowCursor);
}

} // namespace PolyShow
