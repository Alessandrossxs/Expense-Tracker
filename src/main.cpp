#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Expense Tracker"));
    QCoreApplication::setOrganizationName(QStringLiteral("Alessandro"));

    // Preserve the backend's existing relative path for expense_tracker.db.
    MainWindow window;
    window.show();
    return app.exec();
}