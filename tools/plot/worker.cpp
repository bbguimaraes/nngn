#include <iomanip>
#include <iostream>
#include <type_traits>

#include "worker.h"

namespace {

template<typename ...Ts>
auto read_values(const char *name, std::stringstream *s, Ts *...ts) {
    (*s >> ... >> *ts);
    if(s->bad()) {
        std::cerr
            << "PlotWorker::" << name << ": failed to read values from "
            << std::quoted(s->str()) << '\n';
        return false;
    }
    return true;
}

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
    std::stringstream ss;
    ss.write(s.data(), static_cast<std::streamsize>(s.size()));
    std::underlying_type_t<Command> cmd= {};
    if(!read_values("cmd", &ss, &cmd))
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
    std::size_t size = {};
    if(!read_values("cmd_size", ss, &size))
        return false;
    emit this->new_size(size);
    return true;
}

bool PlotWorker::cmd_graph(std::stringstream *ss) {
    std::underlying_type_t<Graph::Type> type = {};
    std::string name;
    if(!read_values("cmd_graph", ss, &type, &name))
        return false;
    if(!check_type("cmd_graph", type))
        return false;
    emit this->new_graph(static_cast<Graph::Type>(type), name.c_str());
    return true;
}

bool PlotWorker::cmd_frame(std::stringstream *ss) {
    qreal timestamp_ms = {};
    if(!read_values("cmd_frame", ss, &timestamp_ms))
        return false;
    emit this->new_frame(timestamp_ms);
    return true;
}

bool PlotWorker::cmd_data(std::stringstream *ss) {
    std::underlying_type_t<Graph::Type> type = {};
    qreal value = {};
    if(!read_values("cmd_data", ss, &type, &value))
        return false;
    if(!check_type("cmd_data", type))
        return false;
    emit this->new_data(static_cast<Graph::Type>(type), value);
    return true;
}
