#include "gui.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GUI w;
    w.setWindowIcon(QIcon(":/256x256.ico"));
    w.show();
    return a.exec();
}
