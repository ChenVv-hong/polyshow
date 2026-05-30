#pragma once

#include <QFrame>

class QLabel;
class QEvent;
class QVBoxLayout;
class QWidget;

namespace PolyShow
{

class MaterialIconLabel;

/// Compact inspector section with attached header and content surfaces.
class InspectorSection final : public QFrame
{
    Q_OBJECT

public:
    /// Creates one titled inspector section.
    explicit InspectorSection(const QString &title, QWidget *parent = nullptr);

    /// Returns the section content layout.
    [[nodiscard]]
    QVBoxLayout *contentLayout() const;

    /// Returns whether the section content is currently hidden.
    [[nodiscard]]
    bool isCollapsed() const;

    /// Updates the visible section title.
    void setTitle(const QString &title);

    /// Shows or hides the section content without destroying editor state.
    void setCollapsed(bool collapsed);

signals:
    /// Emitted when the user or code changes the section collapsed state.
    void collapsedChanged(bool collapsed);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;

    /// Toggles the current collapsed state from header mouse or keyboard input.
    void toggleCollapsed();

    /// Keeps the chevron, content widget, and dynamic QSS properties synchronized.
    void refreshCollapsedState();

    QWidget *m_header_widget {nullptr};
    QWidget *m_content_widget {nullptr};
    MaterialIconLabel *m_chevron_label {nullptr};
    QLabel *m_title_label {nullptr};
    QVBoxLayout *m_content_layout {nullptr};
    bool m_collapsed {false};
};

} // namespace PolyShow
