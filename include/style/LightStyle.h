#pragma once

#include <QPalette>
#include <QProxyStyle>
#include <QString>

namespace PolyShow
{

/// Light application style that mirrors the dark style architecture.
class LightStyle final : public QProxyStyle
{
public:
    /// Creates the light proxy style on top of Fusion.
    LightStyle();

    /// Returns the shared light theme palette.
    [[nodiscard]]
    static QPalette palette();

    /// Returns the light theme stylesheet resource contents.
    [[nodiscard]]
    static QString styleSheet();
};

} // namespace PolyShow
