#include <fcntl.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>

#include "utils/log.h"

#include "graph.h"
#include "worker.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    constexpr int fd = 0;
    if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        return nngn::Log::perror("fctnl(0, F_SETFL, O_NONBLOCK)"), 1;
    qRegisterMetaType<QVector<qreal>>("QVector<qreal>");
    PlotWorker worker;
    if(!worker.init(fd))
        return 1;
    QWidget window;
    auto *const l = new QVBoxLayout(&window);
    auto *const graph = new Graph;
    l->addWidget(graph);
    auto *const mode = new QWidget;
    l->addWidget(mode);
    auto *const mode_layout = new QHBoxLayout(mode);
    auto *const mode_absolute = new QRadioButton("absolute");
    auto *const mode_relative = new QRadioButton("relative");
    mode_layout->addStretch(3.0);
    mode_layout->addWidget(mode_absolute);
    mode_layout->addStretch(1.0);
    mode_layout->addWidget(mode_relative);
    mode_layout->addStretch(3.0);
    switch(graph->mode()) {
    case Graph::Mode::ABSOLUTE: mode_absolute->setChecked(true); break;
    case Graph::Mode::RELATIVE: mode_relative->setChecked(true); break;
    }
    QObject::connect(
        &worker, &PlotWorker::finished,
        &window, &Graph::close);
    QObject::connect(
        &worker, &PlotWorker::new_graph,
        graph, &Graph::new_graph);
    QObject::connect(
        &worker, &PlotWorker::new_data,
        graph, &Graph::new_data);
    QObject::connect(
        &app, &QApplication::lastWindowClosed,
        &worker, &PlotWorker::finish);
    QObject::connect(
        mode_absolute, &QRadioButton::pressed,
        [graph] { graph->set_mode(Graph::Mode::ABSOLUTE); });
    QObject::connect(
        mode_relative, &QRadioButton::pressed,
        [graph] { graph->set_mode(Graph::Mode::RELATIVE); });
    window.resize(800, 600);
    window.setWindowFlags(Qt::Dialog);
    window.show();
    worker.start();
    auto ret = app.exec();
    worker.wait();
    worker.destroy();
    return ret ? ret : !worker.ok();
}
