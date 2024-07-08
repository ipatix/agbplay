#include <QApplication>
#include <QWidget>

#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.setWindowTitle("Hello World Title");
    mainWindow.show();

    return app.exec();
}
