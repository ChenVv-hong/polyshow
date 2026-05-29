#pragma once

#include <QFrame>

class QLabel;
class QVBoxLayout;

namespace PolyShow
{

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

    /// Updates the visible section title.
    void setTitle(const QString &title);

private:
    QLabel *m_title_label {nullptr};
    QVBoxLayout *m_content_layout {nullptr};
};

} // namespace PolyShow
