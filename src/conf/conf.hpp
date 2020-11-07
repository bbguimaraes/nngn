#ifndef IMPERO_CONF_CONF_H
#define IMPERO_CONF_CONF_H

#include <string>
#include <string_view>
#include <vector>

namespace impero {

struct Configuration {
    static constexpr auto APPLICATION_DIR = "impero";
    struct ExecCommandConf { std::string cmd; };
    static std::vector<ExecCommandConf> from_default_file(void);
    static std::vector<ExecCommandConf> from_file(std::string_view path);
};

}

#endif
