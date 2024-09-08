#include "MainWindow.hpp"

#include <portaudiocpp/AutoSystem.hxx>
#include <QApplication>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    portaudio::AutoSystem paSystem;

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
