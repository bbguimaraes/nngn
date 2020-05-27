#ifndef NNGN_SCRIPTS_READER_H
#define NNGN_SCRIPTS_READER_H

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

class LineReader {
    std::string buf = std::string(4096, 0);
    std::stringstream ss = {};
    bool next_line(std::string *s);
public:
    std::ptrdiff_t read(int fd);
    template<typename T> bool for_each(T f);
};

bool LineReader::for_each(auto f) {
    std::string l;
    while(this->next_line(&l))
        if(!f(std::string_view{l}))
            return false;
    return true;
}

#endif
