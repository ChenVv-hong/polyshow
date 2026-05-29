#pragma once

#include <QLabel>
#include <QString>

namespace PolyShow
{

/// Label that displays one Material Symbols Rounded ligature icon.
class MaterialIconLabel final : public QLabel
{
    Q_OBJECT

public:
    /// Creates an icon label with an optional initial ligature name.
    explicit MaterialIconLabel(const QString &iconName = QString(), QWidget *parent = nullptr);

    /// Replaces the displayed Material Symbols ligature name.
    void setIconName(const QString &iconName);

    /// Updates the icon font size and fixed square bounds.
    void setIconPixelSize(int pixelSize);
};

} // namespace PolyShow
