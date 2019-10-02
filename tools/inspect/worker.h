#ifndef NNGN_INSPECT_WORKER_H
#define NNGN_INSPECT_WORKER_H

#include "../worker.h"

class InspectWorker final : public Worker {
    Q_OBJECT
    enum class Command : uint8_t { QUIT = 'q', CLEAR = 'c', LINE = 'l' };
    bool cmd(std::string_view s) final;
    void cmd_line(std::stringstream *ss);
signals:
    void clear();
    void new_line(QString s);
};

#endif
