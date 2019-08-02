#include "worker.h"

#include <iostream>
#include <type_traits>

namespace {

bool check_type(const char *name, std::underlying_type_t<Graph::Type> t) {
    switch(static_cast<Graph::Type>(t)) {
    case Graph::Type::LINE: return true;
    default:
        std::cerr
            << "PlotWorker::" << name << ": invalid graph type: "
            << static_cast<int>(t) << " (" << t << ")\n";
        return false;
    }
}

}

bool PlotWorker::cmd(std::string_view s) {
    NNGN_LOG_CONTEXT_CF(PlotWorker);
    std::stringstream ss;
    ss.write(s.data(), static_cast<std::streamsize>(s.size()));
    std::underlying_type_t<Command> cmd = {};
    if(!Worker::read_values(&ss, &cmd))
        return false;
    switch(static_cast<Command>(cmd)) {
    case Command::QUIT: this->finish(); emit this->finished(); return true;
    case Command::SIZE: return this->cmd_size(&ss);
    case Command::FRAME: return this->cmd_frame(&ss);
    case Command::GRAPH: return this->cmd_graph(&ss);
    case Command::DATA: return this->cmd_data(&ss);
    }
    std::cerr
        << "PlotWorker::cmd: invalid command: "
        << static_cast<int>(cmd) << " (" << cmd << ")\n";
    this->err();
    return false;
}

bool PlotWorker::cmd_size(std::stringstream *ss) {
    NNGN_LOG_CONTEXT_CF(PlotWorker);
    std::size_t size = {};
    if(!Worker::read_values(ss, &size))
        return false;
    emit this->new_size(size);
    return true;
}

bool PlotWorker::cmd_graph(std::stringstream *ss) {
    NNGN_LOG_CONTEXT_CF(PlotWorker);
    std::underlying_type_t<Graph::Type> type = {};
    std::string name;
    if(!Worker::read_values(ss, &type, &name))
        return false;
    if(!check_type("cmd_graph", type))
        return false;
    emit this->new_graph(static_cast<Graph::Type>(type), name.c_str());
    return true;
}

bool PlotWorker::cmd_frame(std::stringstream *ss) {
    NNGN_LOG_CONTEXT_CF(PlotWorker);
    qreal timestamp_ms = {};
    if(!Worker::read_values(ss, &timestamp_ms))
        return false;
    emit this->new_frame(timestamp_ms);
    return true;
}

bool PlotWorker::cmd_data(std::stringstream *ss) {
    NNGN_LOG_CONTEXT_CF(PlotWorker);
    std::underlying_type_t<Graph::Type> type = {};
    qreal value = {};
    if(!Worker::read_values(ss, &type, &value))
        return false;
    if(!check_type("cmd_data", type))
        return false;
    emit this->new_data(static_cast<Graph::Type>(type), value);
    return true;
}
