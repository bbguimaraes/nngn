#ifndef NNGN_COMPUTE_THREAD_POOL_H
#define NNGN_COMPUTE_THREAD_POOL_H

#include <vector>
#include <thread>

namespace nngn {

class ThreadPool {
    std::vector<std::thread> threads = {};
public:
    const std::thread &thread(size_t i) const { return this->threads[i]; }
    std::thread *thread(size_t i) { return &this->threads[i]; }
    template<typename F, typename ...Args> size_t start(F &&f, Args &&...args);
};

template<typename F, typename ...Args>
size_t ThreadPool::start(F &&f, Args &&...args) {
    const auto ret = this->threads.size();
    if constexpr(!sizeof...(Args))
        this->threads.emplace_back(std::forward<F>(f));
    else
        this->threads.emplace_back(
            std::forward<F>(f), std::forward<Args...>(args...));
    return ret;
}

}

#endif
