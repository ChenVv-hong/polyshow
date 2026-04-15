#include "ui/MainWindow.h"
#include "ui/UiTheme.h"

#include <QApplication>
#include <QStyleFactory>

/// Program entry point.
int main(int argc, char *argv[])
{
    // Create the Qt application object and bind command-line arguments.
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("PolyShow"));
    application.setOrganizationName(QStringLiteral("PolyShow"));
    application.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    application.setPalette(PolyShow::UiTheme::palette(PolyShow::ThemeMode::Light));
    application.setStyleSheet(PolyShow::UiTheme::styleSheet(PolyShow::ThemeMode::Light));

    // Create the main window and start the event loop.
    PolyShow::MainWindow mainWindow;
    mainWindow.show();
    return application.exec();
}
