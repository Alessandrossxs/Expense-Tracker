#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Expense Tracker"));
    QCoreApplication::setOrganizationName(QStringLiteral("Alessandro"));

    // Keep the SQLite file in the user's writable application data folder.
    const QString dataDirectory = QDir::home().filePath(QStringLiteral(".expense-tracker"));
    if (!QDir().mkpath(dataDirectory))
        return 1;
    QDir::setCurrent(dataDirectory);

    MainWindow window;
    if (!window.isVisible() && window.windowTitle().isEmpty())
        return 1;
    window.show();
    return app.exec();
}