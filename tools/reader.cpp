#include <algorithm>
#include <cstring>

#include <unistd.h>

#include "utils/log.h"

#include "reader.h"

std::ptrdiff_t LineReader::read(int fd) {
    auto &b = this->buf;
    std::ptrdiff_t ret = 0, n = 0;
    while((n = ::read(fd, b.data(), b.size()))) {
        if(n == -1) {
            if(errno == EAGAIN)
                errno = 0;
            else {
                nngn::Log::perror("read");
                return -1;
            }
            break;
        }
        this->ss.write(b.data(), n);
        ret += n;
    }
    return ret;
}

bool LineReader::next_line(std::string *s) {
    const bool ret = !!std::getline(this->ss, *s);
    if(this->ss.eof()) {
        this->ss.clear();
        this->ss.str({});
    }
    return ret;
}
