#ifndef NNGN_TOOLS_PLOT_WORKER_H
#define NNGN_TOOLS_PLOT_WORKER_H

#include "../worker.h"

#include "graph.h"

class PlotWorker final : public Worker {
    Q_OBJECT
    enum class Command : uint8_t {
        QUIT = 'q', SIZE = 's', GRAPH = 'g', FRAME = 'f', DATA = 'd',
    };
    bool cmd(std::string_view s) final;
    bool cmd_size(std::stringstream *ss);
    bool cmd_graph(std::stringstream *ss);
    bool cmd_frame(std::stringstream *ss);
    bool cmd_data(std::stringstream *ss);
signals:
    void new_size(std::size_t n);
    void new_frame(qreal timestamp_ms);
    void new_graph(Graph::Type type, QString title);
    void new_data(Graph::Type type, qreal value);
};

#endif
