#include <iostream>

#include <QApplication>

#include "utils/log.h"

#include "window.h"

namespace {

void sig(Window *w, QString value, QString cmd) {
    QString s = cmd;
    if(s.indexOf("%1") != -1)
        s = s.arg(value);
    if(std::cout << s.toUtf8().constData() << std::flush)
        return;
    if(errno == EPIPE)
        errno = 0;
    else
        nngn::Log::perror("write");
    w->close();
}

}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    Window w;
    QObject::connect(
        &w, &Window::command_updated,
        [&w](QString value, QString cmd) { sig(&w, value, cmd); });
    if(argc == 1)
        w.add_widget();
    else while(*++argv)
        if(!w.add_widget(*argv))
            return 1;
    w.setWindowFlags(Qt::Dialog);
    w.resize(w.width(), 0);
    w.show();
    return app.exec();
}
