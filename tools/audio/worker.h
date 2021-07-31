#ifndef NNGN_TOOLS_AUDIO_WORKER_H
#define NNGN_TOOLS_AUDIO_WORKER_H

#include "utils/def.h"

#include "../worker.h"

namespace nngn {

class AudioWorker : public Worker {
    Q_OBJECT
signals:
    void new_graph(std::vector<std::byte> v);
    void new_pos(std::size_t p);
private:
    enum class Command : u8 {
        QUIT = 'q', GRAPH = 'g', POS = 'p',
    };
    bool cmd(std::string_view s) final;
    bool cmd_graph(std::stringstream *ss);
    bool cmd_pos(std::stringstream *ss);
};

}

#endif
