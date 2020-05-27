#ifndef NNGN_SCRIPTS_READER_H
#define NNGN_SCRIPTS_READER_H

#include <sstream>
#include <vector>

class LineReader {
    std::vector<char> buf = {};
    std::stringstream ss = std::stringstream{};
    bool next_line(std::string *s);
public:
    LineReader(size_t size) : buf(size) {}
    ssize_t read(int fd);
    template<typename T> bool for_each(T f);
};

template<typename T> bool LineReader::for_each(T f) {
    std::string l;
    while(this->next_line(&l))
        if(!f(static_cast<std::string_view>(l)))
            return false;
    return true;
}

#endif
