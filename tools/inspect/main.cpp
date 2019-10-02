#include <iostream>

#include <QtWidgets/QApplication>

#include "window.h"
#include "worker.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    InspectWorker worker;
    if(!worker.init(0))
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
