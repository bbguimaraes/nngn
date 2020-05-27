#include <iomanip>
#include <iostream>

#include "worker.h"

bool PlotWorker::cmd(std::string_view s) {
    std::stringstream ss;
    ss.write(s.data(), static_cast<std::streamsize>(s.size()));
    uint8_t cmd = {};
    ss >> cmd;
    switch(static_cast<Command>(cmd)) {
    case Command::QUIT: this->finish(); emit this->finished(); return true;
    case Command::GRAPH: this->cmd_graph(&ss); return true;
    case Command::DATA: return this->cmd_data(&ss);
    }
    std::cerr
        << "PlotWorker::cmd: invalid command: "
        << static_cast<int>(cmd) << " (" << cmd << ")\n";
    this->err();
    return false;
}

void PlotWorker::cmd_graph(std::stringstream *ss) {
    for(std::string s; *ss >> s;)
        emit this->new_graph(s.c_str());
}

bool PlotWorker::cmd_data(std::stringstream *ss) {
    QVector<qreal> v;
    for(qreal f; *ss >> f;)
        v << f;
    if(ss->bad()) {
        std::cerr
            << "PlotWorker::cmd_data: failed to read values from "
            << std::quoted(ss->str()) << '\n';
        return false;
    }
    emit this->new_data(v);
    return true;
}
