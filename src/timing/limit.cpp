#include "limit.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace nngn {

void FrameLimiter::limit(void) {
    if(this->m_interval < 1)
        return;
    this->last += this->m_interval * FrameLimiter::duration{1s} / 60;
    if(const auto now = FrameLimiter::clock::now(); now < this->last)
        std::this_thread::sleep_for(this->last - now);
}

}
