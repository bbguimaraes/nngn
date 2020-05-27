#ifndef NNGN_TOOLS_TIMELINE_WORKER_H
#define NNGN_TOOLS_TIMELINE_WORKER_H

#include "../worker.h"

class PlotWorker final : public Worker {
    Q_OBJECT
    enum class Command : uint8_t { QUIT = 'q', GRAPH = 'g', DATA = 'd' };
    bool cmd(std::string_view s) final;
    void cmd_graph(std::stringstream *ss);
    bool cmd_data(std::stringstream *ss);
signals:
    void new_graph(QString title);
    void new_data(const QVector<qreal> &v);
};

#endif
