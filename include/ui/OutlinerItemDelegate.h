#pragma once

#include <QPoint>
#include <QStyledItemDelegate>

namespace PolyShow
{

enum class OutlinerItemHitTarget
{
    None,
    Disclosure,
    Check
};

/// Paints the outliner tree rows to match the HTML prototype grid.
class OutlinerItemDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit OutlinerItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    [[nodiscard]]
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    bool editorEvent(
        QEvent *event,
        QAbstractItemModel *model,
        const QStyleOptionViewItem &option,
        const QModelIndex &index) override;

    [[nodiscard]]
    OutlinerItemHitTarget hitTest(
        const QStyleOptionViewItem &option,
        const QModelIndex &index,
        const QPoint &position) const;

private:
    [[nodiscard]]
    QRect disclosureRect(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    [[nodiscard]]
    QRect checkRect(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    [[nodiscard]]
    QRect iconRect(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    [[nodiscard]]
    QRect labelRect(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

} // namespace PolyShow
