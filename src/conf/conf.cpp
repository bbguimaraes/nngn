#include "conf.hpp"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <QStandardPaths>

namespace {

void err(const char *op, std::string_view path) {
    std::cerr
        << "failed to " << op << " commands file " << std::quoted(path)
        << ": " << std::strerror(errno) << '\n';
}

}

namespace impero {

auto Configuration::from_default_file(void) -> std::vector<ExecCommandConf> {
    auto s = QStandardPaths::locate(
        QStandardPaths::ConfigLocation,
        Configuration::APPLICATION_DIR,
        QStandardPaths::LocateDirectory);
    if(s.isEmpty())
        return {};
    const auto p =
        std::filesystem::path(std::move(s).toStdString())
        / "commands.txt";
    if(!std::filesystem::exists(p))
        return {};
    return Configuration::from_file(std::move(p).string());
}

auto Configuration::from_file(std::string_view path)
    -> std::vector<ExecCommandConf>
{
    std::ifstream f(path.data());
    if(!f) {
        err("open", path);
        return {};
    }
    std::string s = {};
    std::vector<ExecCommandConf> ret;
    while(std::getline(f, s))
        ret.emplace_back(s);
    if(f.bad()) {
        err("read", path);
        return {};
    }
    return ret;
}

}
