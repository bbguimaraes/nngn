#include <cstring>
#include <iostream>

#include <unistd.h>
#include "reader.h"

ssize_t LineReader::read(int fd) {
    const auto ret = ::read(fd, this->buf.data(), this->buf.size());
    if(ret == -1)
        std::cerr << "read: " << std::strerror(errno) << '\n';
    else
        ss.write(this->buf.data(), ret);
    return ret;
}

bool LineReader::next_line(std::string *s) {
    if(std::getline(this->ss, *s) && !this->ss.eof())
        return true;
    this->ss.clear();
    this->ss.str(*s);
    this->ss.seekp(static_cast<std::streamoff>(s->size()));
    return false;
}
