#include "ui/MainWindow.h"
#include "style/AppStyle.h"

#include <QApplication>

/// Program entry point.
int main(int argc, char *argv[])
{
    // Create the Qt application object and bind command-line arguments.
    QApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("PolyShow"));
    application.setOrganizationName(QStringLiteral("PolyShow"));
    PolyShow::AppStyle::install(application, PolyShow::ThemeMode::Dark);

    // Create the main window and start the event loop.
    PolyShow::MainWindow mainWindow;
    mainWindow.show();
    return application.exec();
}
