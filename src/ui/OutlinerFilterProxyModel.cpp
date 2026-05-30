#include "ui/OutlinerFilterProxyModel.h"

#include "ui/OutlinerTreeModel.h"

namespace PolyShow
{

OutlinerFilterProxyModel::OutlinerFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setRecursiveFilteringEnabled(false);
    setDynamicSortFilter(true);
}

void OutlinerFilterProxyModel::setFilterText(const QString &filterText)
{
    const QString normalizedFilterText = filterText.trimmed().toLower();
    if (normalizedFilterText == m_filter_text)
    {
        return;
    }

    m_filter_text = normalizedFilterText;
    invalidateFilter();
}

QString OutlinerFilterProxyModel::filterText() const
{
    return m_filter_text;
}

int OutlinerFilterProxyModel::visibleItemCount() const
{
    return visibleItemCount(QModelIndex());
}

bool OutlinerFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_filter_text.isEmpty())
    {
        return true;
    }

    const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!sourceIndex.isValid())
    {
        return false;
    }

    const QModelIndex parentIndex = sourceIndex.parent();
    if (parentIndex.isValid() && indexMatches(parentIndex))
    {
        return true;
    }

    return indexMatches(sourceIndex) || childMatches(sourceIndex);
}

bool OutlinerFilterProxyModel::indexMatches(const QModelIndex &sourceIndex) const
{
    return sourceIndex.data(OutlinerFilterTextRole).toString().toLower().contains(m_filter_text);
}

bool OutlinerFilterProxyModel::childMatches(const QModelIndex &sourceIndex) const
{
    const int childCount = sourceModel()->rowCount(sourceIndex);
    for (int childRow = 0; childRow < childCount; ++childRow)
    {
        const QModelIndex childIndex = sourceModel()->index(childRow, 0, sourceIndex);
        if (indexMatches(childIndex) || childMatches(childIndex))
        {
            return true;
        }
    }

    return false;
}

int OutlinerFilterProxyModel::visibleItemCount(const QModelIndex &proxyParent) const
{
    int count = 0;
    const int rows = rowCount(proxyParent);
    for (int row = 0; row < rows; ++row)
    {
        const QModelIndex childIndex = index(row, 0, proxyParent);
        ++count;
        count += visibleItemCount(childIndex);
    }

    return count;
}

} // namespace PolyShow
