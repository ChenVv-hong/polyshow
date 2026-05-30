#pragma once

#include <QSortFilterProxyModel>
#include <QString>

namespace PolyShow
{

/// Search proxy matching HTML outliner behavior: parent matches reveal all children.
class OutlinerFilterProxyModel final : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit OutlinerFilterProxyModel(QObject *parent = nullptr);

    void setFilterText(const QString &filterText);

    [[nodiscard]]
    QString filterText() const;

    [[nodiscard]]
    int visibleItemCount() const;

protected:
    [[nodiscard]]
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    [[nodiscard]]
    bool indexMatches(const QModelIndex &sourceIndex) const;

    [[nodiscard]]
    bool childMatches(const QModelIndex &sourceIndex) const;

    [[nodiscard]]
    int visibleItemCount(const QModelIndex &proxyParent) const;

    QString m_filter_text;
};

} // namespace PolyShow
