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
constexpr int kHintCardPadding = 14;
constexpr int kHintCardCornerRadius = 10;
constexpr int kHintCardMaxWidth = 260;
constexpr int kHintCardMinimumWidth = 156;
constexpr int kHintCardGap = 16;
constexpr int kHintCardIconSize = 24;
constexpr int kHintCardIconTextGap = 10;
constexpr int kHintCardRowGap = 8;
constexpr int kHintCardSectionGap = 10;
constexpr qreal kFallbackFitHalfExtent = 250.0;
constexpr int kLayerIndexRole = 1;
constexpr int kPrimitiveIndexRole = 2;
constexpr int kSelectionOverlayRole = 3;

struct DrawingHintContent
{
    QString title;
    QString primary_action;
    QString secondary_action;
    QString footer;
    bool has_secondary_action {false};
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

QString mouseIconResourcePath(bool leftButton, ThemeMode themeMode)
{
    if (leftButton)
    {
        return themeMode == ThemeMode::Dark ? QStringLiteral(":/overlay/mouse_left_click_dark.svg")
                                            : QStringLiteral(":/overlay/mouse_left_click_light.svg");
    }

    return themeMode == ThemeMode::Dark ? QStringLiteral(":/overlay/mouse_right_click_dark.svg")
                                        : QStringLiteral(":/overlay/mouse_right_click_light.svg");
}

DrawingHintContent drawingHintContent(GeometryViewer::ToolMode toolMode)
{
    switch (toolMode)
    {
    case GeometryViewer::ToolMode::DrawPoint:
        return DrawingHintContent {
            QStringLiteral("Point Tool"),
            QStringLiteral("Create point"),
            QString(),
            QStringLiteral("Choose another tool to stop drawing points."),
            false};
    case GeometryViewer::ToolMode::DrawPolyline:
        return DrawingHintContent {
            QStringLiteral("Polyline Tool"),
            QStringLiteral("Add vertex"),
            QStringLiteral("Finish polyline"),
            QStringLiteral("Use Cancel in toolbar to discard draft."),
            true};
    case GeometryViewer::ToolMode::DrawPolygon:
        return DrawingHintContent {
            QStringLiteral("Polygon Tool"),
            QStringLiteral("Add vertex"),
            QStringLiteral("Finish polygon"),
            QStringLiteral("Use Cancel in toolbar to discard draft."),
            true};
    case GeometryViewer::ToolMode::Browse:
    default:
        return {};
    }
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

    if (m_tool_mode == ToolMode::Browse)
    {
        return;
    }

    const int maxHintWidth = std::min(kHintCardMaxWidth, scaleBarRect.left() - kHintCardGap - kHintCardMargin);
    if (maxHintWidth < kHintCardMinimumWidth)
    {
        return;
    }

    const DrawingHintContent hintContent = drawingHintContent(m_tool_mode);
    if (hintContent.title.isEmpty())
    {
        return;
    }

    const RenderColors &renderColors = RenderTheme::colors();
    const ThemeMode themeMode = RenderTheme::activeTheme();

    QFont titleFont = font();
    titleFont.setBold(true);

    QFont bodyFont = font();

    QFont footerFont = font();
    footerFont.setPointSizeF(std::max(footerFont.pointSizeF() - 1.0, 8.0));

    const QFontMetrics titleMetrics(titleFont);
    const QFontMetrics bodyMetrics(bodyFont);
    const QFontMetrics footerMetrics(footerFont);

    const int cardWidth = maxHintWidth;
    const int contentWidth = cardWidth - kHintCardPadding * 2;

    const QRect titleMeasureRect(0, 0, contentWidth, 200);
    const QRect footerMeasureRect(0, 0, contentWidth, 400);
    const QRect titleTextRect = titleMetrics.boundingRect(
        titleMeasureRect,
        Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
        hintContent.title);
    const QRect footerTextRect = footerMetrics.boundingRect(
        footerMeasureRect,
        Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
        hintContent.footer);

    const int actionRowHeight = std::max(bodyMetrics.height(), kHintCardIconSize);
    const int actionCount = hintContent.has_secondary_action ? 2 : 1;
    const int actionsHeight = actionRowHeight * actionCount + kHintCardRowGap * (actionCount - 1);
    const int dividerGap = kHintCardSectionGap;
    const int cardHeight = kHintCardPadding + titleTextRect.height() + kHintCardSectionGap + actionsHeight
        + dividerGap + footerTextRect.height() + kHintCardPadding;

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

    painter->setFont(titleFont);
    painter->setPen(renderColors.overlay_title_text);
    const QRect titleRect(
        hintCardRect.left() + kHintCardPadding,
        cursorY,
        contentWidth,
        titleTextRect.height());
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, hintContent.title);
    cursorY = titleRect.bottom() + 1 + kHintCardSectionGap;

    const QRect primaryRowRect(
        hintCardRect.left() + kHintCardPadding,
        cursorY,
        contentWidth,
        actionRowHeight);
    drawHintActionRow(
        painter,
        primaryRowRect,
        mouseIconResourcePath(true, themeMode),
        hintContent.primary_action,
        renderColors.overlay_body_text,
        bodyFont);
    cursorY = primaryRowRect.bottom() + 1;

    if (hintContent.has_secondary_action)
    {
        cursorY += kHintCardRowGap;
        const QRect secondaryRowRect(
            hintCardRect.left() + kHintCardPadding,
            cursorY,
            contentWidth,
            actionRowHeight);
        drawHintActionRow(
            painter,
            secondaryRowRect,
            mouseIconResourcePath(false, themeMode),
            hintContent.secondary_action,
            renderColors.overlay_body_text,
            bodyFont);
        cursorY = secondaryRowRect.bottom() + 1;
    }

    cursorY += dividerGap / 2;
    painter->setPen(QPen(renderColors.overlay_panel_border));
    painter->drawLine(
        hintCardRect.left() + kHintCardPadding,
        cursorY,
        hintCardRect.right() - kHintCardPadding,
        cursorY);

    cursorY += dividerGap / 2;
    painter->setFont(footerFont);
    painter->setPen(renderColors.overlay_muted_text);
    painter->drawText(
        QRect(
            hintCardRect.left() + kHintCardPadding,
            cursorY,
            contentWidth,
            footerTextRect.height()),
        Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
        hintContent.footer);

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
