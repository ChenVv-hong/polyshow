#include "ui/InspectorCheckBox.h"

#include "style/RenderTheme.h"
#include "ui/MaterialIcon.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QtGlobal>

namespace PolyShow
{

namespace
{

constexpr int kCheckSize = 14;
constexpr int kCheckRadius = 4;
constexpr int kTextGap = 7;
constexpr int kMinimumHeight = 22;

} // namespace

InspectorCheckBox::InspectorCheckBox(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
{
    setObjectName(QStringLiteral("inspectorCheckBox"));
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
}

QSize InspectorCheckBox::sizeHint() const
{
    const QFontMetrics metrics(font());
    return QSize(kCheckSize + kTextGap + metrics.horizontalAdvance(text()), kMinimumHeight);
}

QSize InspectorCheckBox::minimumSizeHint() const
{
    return sizeHint();
}

void InspectorCheckBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const RenderColors &colors = RenderTheme::colors();
    const bool enabled = isEnabled();
    const bool checked = checkState() == Qt::Checked;
    const bool mixed = checkState() == Qt::PartiallyChecked;
    const bool active = checked || mixed;
    const bool hovered = enabled && underMouse();

    QColor borderColor = active ? colors.outliner_check_checked_border : colors.outliner_check_border;
    QColor backgroundColor = active ? colors.outliner_check_checked_background : colors.outliner_check_background;
    QColor markColor = colors.outliner_check_mark;
    QColor textColor = enabled ? colors.outliner_row_text : colors.outliner_row_hidden_text;

    if (hovered && !active)
    {
        borderColor = colors.outliner_check_checked_border;
    }
    else if (hovered)
    {
        backgroundColor = backgroundColor.lighter(112);
    }

    if (!enabled)
    {
        borderColor.setAlpha(130);
        backgroundColor.setAlpha(130);
        markColor.setAlpha(130);
    }

    const QRect checkRect(0, (height() - kCheckSize) / 2, kCheckSize, kCheckSize);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(backgroundColor);
    painter.drawRoundedRect(checkRect.adjusted(0, 0, -1, -1), kCheckRadius, kCheckRadius);

    if (checked)
    {
        // Use the same Material Symbols ligature as the outliner delegate so both checks stay visually aligned.
        QFont checkFont(MaterialIcon::fontFamily());
        checkFont.setPixelSize(13);
        checkFont.setWeight(QFont::Normal);
        painter.setFont(checkFont);
        painter.setPen(markColor);
        painter.drawText(checkRect, Qt::AlignCenter, QStringLiteral("check"));
    }
    else if (mixed)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(markColor);
        const QRect mixedRect(checkRect.center().x() - 4, checkRect.center().y() - 1, 8, 2);
        painter.drawRoundedRect(mixedRect, 1, 1);
    }

    if (hasFocus())
    {
        painter.setPen(QPen(colors.outliner_check_checked_border, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(checkRect.adjusted(1, 1, -2, -2), kCheckRadius - 1, kCheckRadius - 1);
    }

    const QRect textRect(
        checkRect.right() + 1 + kTextGap,
        0,
        qMax(0, width() - checkRect.right() - 1 - kTextGap),
        height());
    painter.setFont(font());
    painter.setPen(textColor);
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
}

} // namespace PolyShow
