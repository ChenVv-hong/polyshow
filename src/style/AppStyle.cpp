#include "style/AppStyle.h"

#include "style/DarkStyle.h"
#include "style/LightStyle.h"
#include "style/RenderTheme.h"

#include <QApplication>

namespace PolyShow
{

void AppStyle::install(QApplication &application, ThemeMode themeMode)
{
    switch (themeMode)
    {
    case ThemeMode::Dark:
        application.setStyle(new DarkStyle());
        application.setPalette(DarkStyle::palette());
        application.setStyleSheet(DarkStyle::styleSheet());
        break;
    case ThemeMode::Light:
    default:
        application.setStyle(new LightStyle());
        application.setPalette(LightStyle::palette());
        application.setStyleSheet(LightStyle::styleSheet());
        break;
    }

    RenderTheme::setActiveTheme(themeMode);
}

} // namespace PolyShow
