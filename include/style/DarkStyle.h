#pragma once

#include <QPalette>
#include <QProxyStyle>
#include <QString>

namespace PolyShow
{

/// Dark application style inspired by the reference project.
class DarkStyle final : public QProxyStyle
{
public:
    /// Creates the dark proxy style on top of Fusion.
    DarkStyle();

    /// Returns the shared dark theme palette.
    [[nodiscard]]
    static QPalette palette();

    /// Returns the dark theme stylesheet resource contents.
    [[nodiscard]]
    static QString styleSheet();
};

} // namespace PolyShow
