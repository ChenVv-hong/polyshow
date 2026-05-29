#pragma once

#include <QPushButton>
#include <QString>

namespace PolyShow
{

/// Reusable rounded button used by compact toolbars.
class PillButton final : public QPushButton
{
    Q_OBJECT

public:
    /// Visual variants used by the shared toolbar buttons.
    enum class Variant
    {
        Neutral,
        Primary,
        Success
    };

    /// Creates one pill button with the shared control styling.
    explicit PillButton(const QString &text = QString(), QWidget *parent = nullptr);

    /// Applies one visual variant to the button.
    void setVariant(Variant variant);

    /// Adds or replaces the Material Symbols Rounded icon shown in the button.
    void setIconName(const QString &iconName);

private:
    /// Maps the enum to the stylesheet property string.
    static QString variantName(Variant variant);
};

} // namespace PolyShow
