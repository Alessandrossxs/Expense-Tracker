#include <QApplication>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Expense Tracker");

    auto *layout = new QVBoxLayout(&window);
    auto *label = new QLabel("Expense Tracker - Qt is working!");

    layout->addWidget(label);

    window.resize(400, 200);
    window.show();

    return app.exec();
}