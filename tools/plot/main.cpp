#include <iostream>

#include <fcntl.h>

#include <QtWidgets/QApplication>

#include "utils/log.h"

#include "graph.h"
#include "window.h"
#include "worker.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    constexpr int fd = 0;
    if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        return nngn::Log::perror("fctnl(0, F_SETFL, O_NONBLOCK)"), 1;
    qRegisterMetaType<QVector<qreal>>("QVector<qreal>");
    qRegisterMetaType<Graph::Type>("Graph::Type");
    qRegisterMetaType<std::size_t>("std::size_t");
    PlotWorker worker;
    if(!worker.init(fd))
        return 1;
    Window window;
    QObject::connect(
        &worker, &PlotWorker::finished,
        &window, &Window::close);
    QObject::connect(
        &worker, &PlotWorker::new_size,
        &window, &Window::set_grid_size);
    QObject::connect(
        &worker, &PlotWorker::new_graph,
        &window, &Window::new_graph);
    QObject::connect(
        &worker, &PlotWorker::new_frame,
        &window, &Window::new_frame);
    QObject::connect(
        &worker, &PlotWorker::new_data,
        &window, &Window::new_data);
    QObject::connect(
        &app, &QApplication::lastWindowClosed,
        &worker, &PlotWorker::finish);
    window.resize(800, 600);
    window.setWindowFlags(Qt::Dialog);
    window.show();
    worker.start();
    auto ret = app.exec();
    worker.wait();
    worker.destroy();
    return ret ? ret : !worker.ok();
}
