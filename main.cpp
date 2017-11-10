#include "gamma.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    grav w;
    w.setWindowFlags(Qt::Window);
    w.setWindowTitle("gamma-ray (v.1.9)");
    //w.showMaximized();
    w.setWindowIcon(QIcon(":/untitled.png"));
    w.show();

    return a.exec();
}

