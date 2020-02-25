#ifndef NNGN_COMPUTE_THREAD_POOL_H
#define NNGN_COMPUTE_THREAD_POOL_H

#include <vector>
#include <thread>

#include "utils/utils.h"

namespace nngn {

class ThreadPool {
public:
    const std::jthread &thread(std::size_t i) const { return this->threads[i]; }
    std::jthread &thread(std::size_t i) { return this->threads[i]; }
    std::size_t start(auto &&f, auto &&...args);
    void stop(void);
private:
    std::vector<std::jthread> threads = {};
};

inline std::size_t ThreadPool::start(auto &&f, auto &&...args) {
    const auto ret = this->threads.size();
    if constexpr(!sizeof...(args))
        this->threads.emplace_back(FWD(f));
    else
        this->threads.emplace_back(FWD(f), FWD(args)...);
    return ret;
}

inline void ThreadPool::stop(void) {
    for(auto &x: this->threads)
        x.request_stop();
}

}

#endif
