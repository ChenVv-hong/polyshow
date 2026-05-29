#pragma once

#include <QWidget>

class QHBoxLayout;
class QLabel;

namespace PolyShow
{

class MaterialIconLabel;

/// Shared compact panel header used by outliner, inspector, and log panels.
class EditorPanelHeader final : public QWidget
{
    Q_OBJECT

public:
    /// Creates a header with one Material icon and title.
    explicit EditorPanelHeader(const QString &iconName, const QString &title, QWidget *parent = nullptr);

    /// Returns the trailing layout for action buttons or chips.
    [[nodiscard]]
    QHBoxLayout *actionsLayout() const;

private:
    MaterialIconLabel *m_icon_label {nullptr};
    QLabel *m_title_label {nullptr};
    QHBoxLayout *m_actions_layout {nullptr};
};

} // namespace PolyShow
