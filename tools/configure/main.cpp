#include <iostream>

#include <QApplication>

#include "window.h"

namespace {

void sig(const QString &value, const QString &cmd) {
    QString s = cmd;
    if(s.indexOf("%1") != -1)
        s = s.arg(value);
    std::cout << s.toUtf8().constData();
    std::cout.flush();
}

}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    Window w;
    QObject::connect(&w, &Window::command_updated, sig);
    w.show();
    return app.exec();
}
