#pragma once

#include "style/ThemeMode.h"

class QApplication;

namespace PolyShow
{

/// Installs the global Qt widget style, palette, and QSS resources.
class AppStyle final
{
public:
    /// Applies the chosen application style to the Qt application instance.
    static void install(QApplication &application, ThemeMode themeMode);
};

} // namespace PolyShow
