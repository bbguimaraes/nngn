#ifndef NNGN_TOOLS_PLOT_WINDOW_H
#define NNGN_TOOLS_PLOT_WINDOW_H

#include <vector>
#include <QVector>
#include <QtWidgets/QMainWindow>

#include "graph.h"

namespace QtCharts {
    class QAbstractSeries;
}

class Window : public QMainWindow {
    Q_OBJECT
    std::vector<QtCharts::QAbstractSeries*> series = {};
    qreal timestamp_ms = {};
    std::size_t i = {}, cols = 4;
public:
    Window();
public slots:
    void set_grid_size(std::size_t n_cols) { this->cols = n_cols; }
    void new_graph(Graph::Type type, QString title);
    void new_frame(qreal timestamp_ms);
    void new_data(Graph::Type type, qreal value);
};

#endif
