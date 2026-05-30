#include "ui/LayerSidebar.h"

#include "ui/EditorPanelHeader.h"
#include "ui/IconButton.h"
#include "ui/MaterialIconLabel.h"
#include "ui/OutlinerFilterProxyModel.h"
#include "ui/OutlinerItemDelegate.h"
#include "ui/OutlinerTreeModel.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyleOptionViewItem>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

namespace PolyShow
{

namespace
{

bool hasSameOutlinerShape(const DocumentData &left, const DocumentData &right)
{
    if (left.layers.size() != right.layers.size())
    {
        return false;
    }

    for (int layerIndex = 0; layerIndex < left.layers.size(); ++layerIndex)
    {
        if (left.layers.at(layerIndex).primitives.size() != right.layers.at(layerIndex).primitives.size())
        {
            return false;
        }
    }

    return true;
}

QWidget *createEmptyStateWidget(QWidget *parent)
{
    auto *emptyWidget = new QWidget(parent);
    emptyWidget->setObjectName(QStringLiteral("outlinerEmpty"));
    emptyWidget->setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(emptyWidget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignCenter);

    auto *icon = new MaterialIconLabel(QStringLiteral("folder_open"), emptyWidget);
    icon->setProperty("iconRole", QStringLiteral("empty"));
    icon->setIconPixelSize(30);
    layout->addWidget(icon, 0, Qt::AlignCenter);

    auto *title = new QLabel(QStringLiteral("No layers"), emptyWidget);
    title->setObjectName(QStringLiteral("outlinerEmptyTitle"));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title, 0, Qt::AlignCenter);

    auto *body = new QLabel(QStringLiteral("Open .ply or create a layer"), emptyWidget);
    body->setObjectName(QStringLiteral("outlinerEmptyBody"));
    body->setAlignment(Qt::AlignCenter);
    layout->addWidget(body, 0, Qt::AlignCenter);

    return emptyWidget;
}

} // namespace

LayerSidebar::LayerSidebar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("outliner"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new EditorPanelHeader(QStringLiteral("account_tree"), QStringLiteral("Outliner"), this);

    m_new_layer_button = new IconButton(QStringLiteral("add"), QString(), header);
    m_new_layer_button->setObjectName(QStringLiteral("iconButton"));
    m_new_layer_button->setCursor(Qt::PointingHandCursor);
    m_new_layer_button->setToolTip(QStringLiteral("Create Layer"));
    header->actionsLayout()->addWidget(m_new_layer_button);

    m_export_layer_button = new IconButton(QStringLiteral("ios_share"), QString(), header);
    m_export_layer_button->setObjectName(QStringLiteral("iconButton"));
    m_export_layer_button->setCursor(Qt::PointingHandCursor);
    m_export_layer_button->setToolTip(QStringLiteral("Export Active Layer"));
    header->actionsLayout()->addWidget(m_export_layer_button);

    m_search_button = new IconButton(QStringLiteral("search"), QString(), header);
    m_search_button->setObjectName(QStringLiteral("iconButton"));
    m_search_button->setCheckable(true);
    m_search_button->setCursor(Qt::PointingHandCursor);
    m_search_button->setToolTip(QStringLiteral("Search"));
    header->actionsLayout()->addWidget(m_search_button);
    layout->addWidget(header);

    m_search_line_edit = new QLineEdit(this);
    m_search_line_edit->setObjectName(QStringLiteral("outlinerSearchInput"));
    m_search_line_edit->setPlaceholderText(QStringLiteral("Search layers and primitives"));
    m_search_line_edit->setVisible(false);
    layout->addWidget(m_search_line_edit);

    m_tree_model = new OutlinerTreeModel(this);
    m_filter_model = new OutlinerFilterProxyModel(this);
    m_filter_model->setSourceModel(m_tree_model);

    m_tree_view = new QTreeView(this);
    m_tree_view->setObjectName(QStringLiteral("outlinerTree"));
    m_tree_view->setModel(m_filter_model);
    m_tree_view->setItemDelegate(new OutlinerItemDelegate(m_tree_view));
    m_tree_view->setHeaderHidden(true);
    m_tree_view->setRootIsDecorated(false);
    m_tree_view->setItemsExpandable(true);
    m_tree_view->setExpandsOnDoubleClick(true);
    m_tree_view->setUniformRowHeights(true);
    m_tree_view->setAllColumnsShowFocus(false);
    m_tree_view->setIndentation(0);
    m_tree_view->setMouseTracking(true);
    m_tree_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tree_view->viewport()->installEventFilter(this);
    layout->addWidget(m_tree_view, 1);

    m_empty_widget = createEmptyStateWidget(this);
    m_empty_widget->setVisible(false);
    layout->addWidget(m_empty_widget, 1);

    m_footer_label = new QLabel(this);
    m_footer_label->setObjectName(QStringLiteral("outlinerSummary"));
    m_footer_label->setProperty("role", QStringLiteral("mono"));
    m_footer_label->setFixedHeight(26);
    layout->addWidget(m_footer_label);

    connect(m_search_button, &QPushButton::toggled, this, [this](bool checked) {
        setSearchExpanded(checked);
    });
    connect(m_new_layer_button, &QPushButton::clicked, this, &LayerSidebar::createLayerRequested);
    connect(m_export_layer_button, &QPushButton::clicked, this, &LayerSidebar::exportLayerRequested);
    connect(m_search_line_edit, &QLineEdit::textChanged, this, [this]() {
        applyFilter(!m_search_line_edit->text().trimmed().isEmpty());
    });
    connect(
        m_tree_model,
        &OutlinerTreeModel::layerVisibilityChangeRequested,
        this,
        &LayerSidebar::layerVisibilityChanged);
    connect(
        m_tree_model,
        &OutlinerTreeModel::primitiveVisibilityChangeRequested,
        this,
        &LayerSidebar::primitiveVisibilityChanged);
    connect(m_filter_model, &QAbstractItemModel::modelReset, this, [this]() {
        updateContentSurface();
        setSelectionState(m_selection_state);
    });
    connect(m_filter_model, &QAbstractItemModel::rowsInserted, this, [this]() {
        updateContentSurface();
    });
    connect(m_filter_model, &QAbstractItemModel::rowsRemoved, this, [this]() {
        updateContentSurface();
    });
    connect(
        m_tree_view->selectionModel(),
        &QItemSelectionModel::currentChanged,
        this,
        [this](const QModelIndex &current) {
            if (m_is_syncing_selection || m_is_applying_filter)
            {
                return;
            }

            if (!current.isValid())
            {
                m_selection_state = SelectionState {};
                emit selectionChanged(m_selection_state);
                return;
            }

            const QModelIndex sourceIndex = m_filter_model->mapToSource(current);
            const SelectionState nextSelection = m_tree_model->selectionForIndex(sourceIndex);
            if (nextSelection != m_selection_state)
            {
                m_selection_state = nextSelection;
                emit selectionChanged(m_selection_state);
            }
        });

    updateContentSurface();
    updateFooter();
}

bool LayerSidebar::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_tree_view->viewport())
    {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() != Qt::LeftButton)
        {
            return QWidget::eventFilter(watched, event);
        }

        const QModelIndex index = m_tree_view->indexAt(mouseEvent->pos());
        auto *delegate = qobject_cast<OutlinerItemDelegate *>(m_tree_view->itemDelegate(index));
        if (delegate == nullptr)
        {
            return QWidget::eventFilter(watched, event);
        }

        QStyleOptionViewItem option;
        option.rect = m_tree_view->visualRect(index);
        const OutlinerItemHitTarget hitTarget = delegate->hitTest(option, index, mouseEvent->pos());
        if (hitTarget == OutlinerItemHitTarget::None)
        {
            return QWidget::eventFilter(watched, event);
        }

        // Consume tree-control presses before QTreeView changes currentIndex; visibility toggles must not select rows.
        m_has_tree_control_press = true;
        m_tree_control_press_index = index;
        m_tree_control_press_target = static_cast<int>(hitTarget);
        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease && m_has_tree_control_press)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPersistentModelIndex pressedIndex = m_tree_control_press_index;
        const auto pressedTarget = static_cast<OutlinerItemHitTarget>(m_tree_control_press_target);
        m_has_tree_control_press = false;
        m_tree_control_press_index = QPersistentModelIndex();
        m_tree_control_press_target = static_cast<int>(OutlinerItemHitTarget::None);

        if (mouseEvent->button() != Qt::LeftButton || !pressedIndex.isValid())
        {
            return true;
        }

        const QModelIndex index = m_tree_view->indexAt(mouseEvent->pos());
        if (index != QModelIndex(pressedIndex))
        {
            return true;
        }

        auto *delegate = qobject_cast<OutlinerItemDelegate *>(m_tree_view->itemDelegate(index));
        if (delegate == nullptr)
        {
            return true;
        }

        QStyleOptionViewItem option;
        option.rect = m_tree_view->visualRect(index);
        if (delegate->hitTest(option, index, mouseEvent->pos()) != pressedTarget)
        {
            return true;
        }

        if (pressedTarget == OutlinerItemHitTarget::Disclosure)
        {
            m_tree_view->setExpanded(index, !m_tree_view->isExpanded(index));
            return true;
        }

        if (pressedTarget == OutlinerItemHitTarget::Check)
        {
            const Qt::CheckState currentState = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
            const Qt::CheckState nextState = currentState == Qt::Checked ? Qt::Unchecked : Qt::Checked;
            m_filter_model->setData(index, nextState, Qt::CheckStateRole);
            return true;
        }

        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void LayerSidebar::setDocumentData(const DocumentData &documentData, bool rebuildTreeItems)
{
    const bool shouldExpandLayers = rebuildTreeItems || !hasSameOutlinerShape(m_document_data, documentData);
    m_document_data = documentData;
    m_tree_model->setDocumentData(documentData, rebuildTreeItems);
    applyFilter(shouldExpandLayers || !m_search_line_edit->text().trimmed().isEmpty());
    updateContentSurface();
    updateFooter();
    setSelectionState(m_selection_state);
}

void LayerSidebar::setSelectionState(const SelectionState &selectionState)
{
    m_is_syncing_selection = true;
    m_selection_state = selectionState;

    const QSignalBlocker selectionBlocker(m_tree_view->selectionModel());
    const QModelIndex sourceIndex = m_tree_model->indexForSelection(selectionState);
    const QModelIndex proxyIndex = sourceIndex.isValid() ? m_filter_model->mapFromSource(sourceIndex) : QModelIndex();

    if (!proxyIndex.isValid())
    {
        m_tree_view->selectionModel()->clearSelection();
        m_tree_view->setCurrentIndex(QModelIndex());
        m_is_syncing_selection = false;
        return;
    }

    m_tree_view->setCurrentIndex(proxyIndex);
    m_tree_view->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    m_tree_view->scrollTo(proxyIndex, QAbstractItemView::PositionAtCenter);

    m_is_syncing_selection = false;
}

void LayerSidebar::setSearchExpanded(bool expanded)
{
    m_is_search_expanded = expanded;
    m_search_line_edit->setVisible(expanded && !m_document_data.isEmpty());

    if (!expanded)
    {
        const QSignalBlocker blocker(m_search_line_edit);
        m_search_line_edit->clear();
    }
    else if (!m_document_data.isEmpty())
    {
        m_search_line_edit->setFocus(Qt::OtherFocusReason);
    }

    applyFilter(expanded && !m_search_line_edit->text().trimmed().isEmpty());
    updateContentSurface();
}

void LayerSidebar::updateFooter()
{
    const QString query = m_filter_model->filterText();
    if (m_is_search_expanded && !query.isEmpty())
    {
        m_footer_label->setText(
            QStringLiteral("Query: %1 / %2 matches").arg(query).arg(m_filter_model->visibleItemCount()));
        return;
    }

    m_footer_label->setText(
        QStringLiteral("%1 layers / %2 visible primitives")
            .arg(m_tree_model->layerCount())
            .arg(m_tree_model->visiblePrimitiveCount()));
}

void LayerSidebar::applyFilter(bool expandVisibleLayersAfterFilter)
{
    m_is_applying_filter = true;
    m_filter_model->setFilterText(m_search_line_edit->text());
    m_is_applying_filter = false;
    if (expandVisibleLayersAfterFilter)
    {
        expandVisibleLayers();
    }
    updateFooter();
    updateContentSurface();
    setSelectionState(m_selection_state);
}

void LayerSidebar::updateContentSurface()
{
    const bool hasLayers = !m_document_data.isEmpty();
    m_tree_view->setVisible(hasLayers);
    m_empty_widget->setVisible(!hasLayers);

    if (!hasLayers && m_is_search_expanded)
    {
        const QSignalBlocker buttonBlocker(m_search_button);
        m_search_button->setChecked(false);
        m_is_search_expanded = false;
        m_search_line_edit->setVisible(false);
        m_filter_model->setFilterText(QString());
    }
}

void LayerSidebar::expandVisibleLayers()
{
    for (int row = 0; row < m_filter_model->rowCount(); ++row)
    {
        m_tree_view->expand(m_filter_model->index(row, 0));
    }
}

} // namespace PolyShow
