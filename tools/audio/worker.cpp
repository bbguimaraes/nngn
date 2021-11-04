#include "worker.h"

#include <iostream>
#include <sstream>

#include "utils/log.h"
#include "utils/utils.h"

namespace nngn {

bool AudioWorker::cmd(std::string_view s) {
    NNGN_LOG_CONTEXT_CF(AudioWorker);
    std::stringstream ss;
    ss.write(s.data(), static_cast<std::streamsize>(s.size()));
    std::underlying_type_t<Command> cmd = {};
    if(!Worker::read_values(&ss, &cmd))
        return false;
    switch(static_cast<Command>(cmd)) {
    case Command::QUIT: emit this->finished(); return true;
    case Command::GRAPH: return this->cmd_graph(&ss);
    case Command::POS: return this->cmd_pos(&ss);
    }
    nngn::Log::l()
        << "invalid command: " << static_cast<int>(cmd)
        << " (" << cmd << ")\n";
    this->err();
    return false;
}

bool AudioWorker::cmd_graph(std::stringstream *ss) {
    NNGN_LOG_CONTEXT_CF(AudioWorker);
    std::size_t n = {};
    if(!Worker::read_values(ss, &n))
        return false;
    auto v = std::vector<std::byte>(n);
    std::cin.read(
        nngn::byte_cast<char*>(v.data()),
        static_cast<std::streamsize>(n));
    emit this->new_graph(std::move(v));
    return true;
}

bool AudioWorker::cmd_pos(std::stringstream *ss) {
    NNGN_LOG_CONTEXT_CF(AudioWorker);
    std::size_t i = {}, p = {};
    if(!Worker::read_values(ss, &i, &p))
        return false;
    emit this->new_pos(i, p);
    return true;
}

}
