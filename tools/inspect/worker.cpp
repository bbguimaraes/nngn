#include <sstream>

#include "utils/log.h"

#include "worker.h"

bool InspectWorker::cmd(std::string_view s) {
    NNGN_LOG_CONTEXT_CF(InspectWorker);
    std::stringstream ss;
    ss.write(s.data(), static_cast<std::streamsize>(s.size()));
    uint8_t cmd = {};
    ss >> cmd;
    switch(static_cast<Command>(cmd)) {
    case Command::QUIT: emit this->finished(); return true;
    case Command::CLEAR: emit this->clear(); return true;
    case Command::LINE: this->cmd_line(&ss); return true;
    }
    nngn::Log::l()
        << "invalid command: "
        << static_cast<int>(cmd) << " (" << cmd << ")\n";
    this->err();
    return false;
}

void InspectWorker::cmd_line(std::stringstream *ss) {
    std::string l;
    std::getline(*ss, l);
    emit this->new_line(l.c_str());
}
