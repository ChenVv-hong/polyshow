#include "ui/OutlinerItemDelegate.h"

#include "style/RenderTheme.h"
#include "ui/MaterialIcon.h"
#include "ui/OutlinerTreeModel.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QTreeView>

namespace PolyShow
{

namespace
{

constexpr int kRowHeight = 25;
constexpr int kHorizontalPadding = 6;
constexpr int kChildIndent = 16;
constexpr int kDisclosureSize = 16;
constexpr int kIconSize = 17;
constexpr int kCheckSize = 14;
constexpr int kGap = 6;

bool hasChildren(const QModelIndex &index)
{
    return index.model() != nullptr && index.model()->rowCount(index) > 0;
}

bool isPrimitive(const QModelIndex &index)
{
    return index.data(OutlinerNodeKindRole).toInt() == static_cast<int>(OutlinerNodeKind::Primitive);
}

QRect rowRect(const QStyleOptionViewItem &option)
{
    return option.rect.adjusted(kHorizontalPadding, 0, -kHorizontalPadding, 0);
}

} // namespace

OutlinerItemDelegate::OutlinerItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void OutlinerItemDelegate::paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    const RenderColors &colors = RenderTheme::colors();
    const bool selected = option.state.testFlag(QStyle::State_Selected)
        || index.data(OutlinerSelectedRole).toBool();
    const bool hovered = option.state.testFlag(QStyle::State_MouseOver);
    const bool hidden = index.data(OutlinerHiddenRole).toBool();
    const Qt::CheckState checkState = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());

    painter->fillRect(option.rect, colors.outliner_row_background);
    if (selected)
    {
        painter->fillRect(option.rect, colors.outliner_row_selected_background);
    }
    else if (hovered)
    {
        painter->fillRect(option.rect, colors.outliner_row_hover_background);
    }

    const QColor textColor = selected ? colors.outliner_row_selected_text
        : (hidden ? colors.outliner_row_hidden_text : colors.outliner_row_text);
    const QColor iconColor = selected ? colors.outliner_icon_selected
        : (hidden ? colors.outliner_row_hidden_text : colors.outliner_icon);

    if (!isPrimitive(index) && hasChildren(index))
    {
        QFont disclosureFont(MaterialIcon::fontFamily());
        disclosureFont.setPixelSize(16);
        disclosureFont.setWeight(QFont::Normal);
        painter->setFont(disclosureFont);
        painter->setPen(iconColor);
        painter->drawText(
            disclosureRect(option, index),
            Qt::AlignCenter,
            option.state.testFlag(QStyle::State_Open) ? QStringLiteral("expand_more")
                                                      : QStringLiteral("chevron_right"));
    }

    QFont iconFont(MaterialIcon::fontFamily());
    iconFont.setPixelSize(17);
    iconFont.setWeight(QFont::Normal);
    painter->setFont(iconFont);
    painter->setPen(iconColor);
    painter->drawText(iconRect(option, index), Qt::AlignCenter, index.data(OutlinerIconNameRole).toString());

    const QRect checkBoxRect = checkRect(option, index);
    painter->setPen(QPen(
        checkState == Qt::Unchecked ? colors.outliner_check_border : colors.outliner_check_checked_border,
        1));
    painter->setBrush(checkState == Qt::Unchecked ? colors.outliner_check_background
                                                   : colors.outliner_check_checked_background);
    painter->drawRoundedRect(checkBoxRect.adjusted(0, 0, -1, -1), 4, 4);

    if (checkState == Qt::Checked)
    {
        QFont checkFont(MaterialIcon::fontFamily());
        checkFont.setPixelSize(13);
        checkFont.setWeight(QFont::Normal);
        painter->setFont(checkFont);
        painter->setPen(colors.outliner_check_mark);
        painter->drawText(checkBoxRect, Qt::AlignCenter, QStringLiteral("check"));
    }
    else if (checkState == Qt::PartiallyChecked)
    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(colors.outliner_check_mark);
        const QRect mixedRect(
            checkBoxRect.center().x() - 4,
            checkBoxRect.center().y() - 1,
            8,
            2);
        painter->drawRoundedRect(mixedRect, 1, 1);
    }

    QFont labelFont = option.font;
    labelFont.setWeight(selected && isPrimitive(index) ? QFont::DemiBold : QFont::Normal);
    if (!isPrimitive(index))
    {
        labelFont.setWeight(QFont::DemiBold);
    }

    painter->setFont(labelFont);
    painter->setPen(textColor);
    const QFontMetrics labelMetrics(labelFont);
    const QRect textRect = labelRect(option, index);
    const QString labelText = labelMetrics.elidedText(
        index.data(Qt::DisplayRole).toString(),
        Qt::ElideRight,
        textRect.width());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, labelText);

    painter->restore();
}

QSize OutlinerItemDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return QSize(0, kRowHeight);
}

bool OutlinerItemDelegate::editorEvent(
    QEvent *event,
    QAbstractItemModel *model,
    const QStyleOptionViewItem &option,
    const QModelIndex &index)
{
    if (model == nullptr || !index.isValid())
    {
        return false;
    }

    if (event->type() == QEvent::MouseButtonRelease)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() != Qt::LeftButton)
        {
            return false;
        }

        if (!isPrimitive(index) && hasChildren(index) && disclosureRect(option, index).contains(mouseEvent->pos()))
        {
            auto *treeView = qobject_cast<QTreeView *>(parent());
            if (treeView == nullptr)
            {
                return false;
            }

            treeView->setExpanded(index, !treeView->isExpanded(index));
            return true;
        }

        if (!checkRect(option, index).contains(mouseEvent->pos()))
        {
            return false;
        }
    }
    else if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() != Qt::Key_Space && keyEvent->key() != Qt::Key_Select)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    const Qt::CheckState currentState = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
    const Qt::CheckState nextState = currentState == Qt::Checked ? Qt::Unchecked : Qt::Checked;
    return model->setData(index, nextState, Qt::CheckStateRole);
}

OutlinerItemHitTarget OutlinerItemDelegate::hitTest(
    const QStyleOptionViewItem &option,
    const QModelIndex &index,
    const QPoint &position) const
{
    if (!index.isValid())
    {
        return OutlinerItemHitTarget::None;
    }

    if (!isPrimitive(index) && hasChildren(index) && disclosureRect(option, index).contains(position))
    {
        return OutlinerItemHitTarget::Disclosure;
    }

    if (checkRect(option, index).contains(position))
    {
        return OutlinerItemHitTarget::Check;
    }

    return OutlinerItemHitTarget::None;
}

QRect OutlinerItemDelegate::disclosureRect(const QStyleOptionViewItem &option, const QModelIndex &) const
{
    const QRect bounds = rowRect(option);
    return QRect(
        bounds.left(),
        option.rect.center().y() - kDisclosureSize / 2,
        kDisclosureSize,
        kDisclosureSize);
}

QRect OutlinerItemDelegate::checkRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QRect iconBounds = iconRect(option, index);
    return QRect(
        iconBounds.right() + 1 + kGap,
        option.rect.center().y() - kCheckSize / 2,
        kCheckSize,
        kCheckSize);
}

QRect OutlinerItemDelegate::iconRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QRect bounds = rowRect(option);
    const int left = bounds.left() + kDisclosureSize + (isPrimitive(index) ? kChildIndent : 0);
    return QRect(left, option.rect.center().y() - kIconSize / 2, kIconSize, kIconSize);
}

QRect OutlinerItemDelegate::labelRect(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QRect checkBoxRect = checkRect(option, index);
    const int left = checkBoxRect.right() + 1 + kGap;
    return QRect(
        left,
        option.rect.top(),
        qMax(0, option.rect.right() - left - kHorizontalPadding + 1),
        option.rect.height());
}

} // namespace PolyShow
