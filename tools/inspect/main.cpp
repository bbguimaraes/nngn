#include <iostream>

#include <fcntl.h>

#include <QtWidgets/QApplication>

#include "window.h"
#include "worker.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    constexpr int fd = 0;
    if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        return nngn::Log::perror("fctnl(0, F_SETFL, O_NONBLOCK)"), 1;
    InspectWorker worker;
    if(!worker.init(fd))
        return 1;
    Window window;
    QObject::connect(
        &worker, &InspectWorker::finished,
        &window, &Window::close);
    QObject::connect(
        &worker, &InspectWorker::clear,
        &window, &Window::clear);
    QObject::connect(
        &worker, &InspectWorker::new_line,
        &window, &Window::new_line);
    QObject::connect(
        &app, &QApplication::lastWindowClosed,
        &worker, &InspectWorker::finish);
    window.resize(400, 600);
    window.setWindowFlags(Qt::Dialog);
    window.show();
    worker.start();
    auto ret = app.exec();
    worker.wait();
    worker.destroy();
    return ret ? ret : !worker.ok();
}
