#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile font_file(":/Menlo-Regular.ttf");
    if (font_file.open(QIODevice::ReadOnly)) {
        int font_id = QFontDatabase::addApplicationFontFromData(font_file.readAll());
        if (font_id < 0) {
            qDebug() << "load menlo font failed";
        } else {
            QFont font = QFont("Menlo", 10);
            a.setFont(font);
        }

        font_file.close();
    } else {
        qDebug() << "open menlo font file failed";
    }

    MainWindow w;
    w.show();

    return a.exec();
}
