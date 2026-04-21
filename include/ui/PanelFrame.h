#pragma once

#include <QFrame>
#include <QString>

namespace PolyShow
{

/// Shared framed container used throughout the UI.
class PanelFrame final : public QFrame
{
    Q_OBJECT

public:
    /// Supported surface variants for reusable panel containers.
    enum class Variant
    {
        Panel,
        Card,
        Canvas
    };

    /// Creates a framed panel with the chosen variant.
    explicit PanelFrame(Variant variant = Variant::Panel, QWidget *parent = nullptr);

    /// Updates the panel surface variant.
    void setVariant(Variant variant);

private:
    /// Maps the enum to the stylesheet property string.
    static QString variantName(Variant variant);
};

} // namespace PolyShow
