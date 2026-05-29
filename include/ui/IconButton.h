#pragma once

#include <QPushButton>
#include <QString>

namespace PolyShow
{

/// Compact push button that uses Material Symbols Rounded for its icon.
class IconButton final : public QPushButton
{
    Q_OBJECT

public:
    /// Creates an icon-only or icon-plus-text button.
    explicit IconButton(const QString &iconName, const QString &text = QString(), QWidget *parent = nullptr);

    /// Replaces the displayed icon.
    void setIconName(const QString &iconName);
};

} // namespace PolyShow
