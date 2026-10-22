#include "Logo.h"
#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QStandardPaths>

namespace
{
// The database lives in the per-user application data folder, so it no longer
// depends on the directory the program is started from. A database found in the
// current directory (older versions) is copied there on first launch.
QString resolveDatabasePath()
{
    const QString fileName = QStringLiteral("expense_tracker.db");
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty() || !QDir().mkpath(dataDir))
        return QDir::current().filePath(fileName);

    const QString target = QDir(dataDir).filePath(fileName);
    const QString legacy = QDir::current().filePath(fileName);
    if (!QFileInfo::exists(target) && QFileInfo::exists(legacy))
        QFile::copy(legacy, target);
    return target;
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Expense Tracker"));
    QCoreApplication::setOrganizationName(QStringLiteral("Alessandro"));

    app.setWindowIcon(appIcon(QColor(QStringLiteral("#6658d3"))));

    // Prefer Segoe UI, but fall back gracefully on macOS and Linux.
    QFont font = app.font();
    font.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("SF Pro Text"),
                      QStringLiteral("Helvetica Neue"), QStringLiteral("Noto Sans"),
                      QStringLiteral("Ubuntu"), QStringLiteral("Cantarell")});
    app.setFont(font);

    MainWindow window(resolveDatabasePath());
    window.showMaximized(); // fills the screen; the title bar and taskbar stay available
    return app.exec();
}
