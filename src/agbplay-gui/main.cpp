#include <QApplication>
#include <QWidget>

#include <portaudiocpp/AutoSystem.hxx>

#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    portaudio::AutoSystem paSystem;

    MainWindow mainWindow;
    mainWindow.setWindowTitle("Hello World Title");
    mainWindow.show();

    return app.exec();
}
