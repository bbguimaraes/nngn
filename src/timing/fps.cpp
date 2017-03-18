#include <cassert>
#include <deque>
#include <iomanip>
#include <limits>
#include <sstream>

#include "fps.h"

namespace nngn {

FPS::FPS(frame_queue::size_type n)
    : avg_hist(frame_queue::container_type(n))
    { assert(!this->avg_hist.empty()); }

void FPS::init(Timing::clock::time_point t) {
    this->last_f = this->last_t = t;
    this->reset_min_max();
}

void FPS::frame(Timing::clock::time_point t) {
    const auto update_avg = [](auto s, auto *f, const auto &dt) {
        s -= f->front();
        f->pop();
        return s + f->emplace(dt);
    };
    const auto dt = t - this->last_f;
    this->last_f = t;
    this->min_dt = std::min(this->min_dt, dt);
    this->max_dt = std::max(this->max_dt, dt);
    this->avg_sum = update_avg(this->avg_sum, &this->avg_hist, dt);
    this->avg = std::chrono::duration<float>(this->avg_hist.size())
        / std::chrono::duration<float>(this->avg_sum);
    if(const auto s = t - this->last_t;
            !std::chrono::duration_cast<std::chrono::seconds>(s).count())
        ++this->sec_count;
    else {
        this->last_t = t;
        this->sec_last = this->sec_count;
        this->sec_count = 0;
    }
}

void FPS::reset_min_max() {
    this->min_dt = Timing::clock::duration(
        std::numeric_limits<Timing::clock::rep>::max());
    this->max_dt = {};
}

std::string FPS::to_string() const {
    const auto cast = [](const auto &d) {
        using D = std::chrono::duration<float, std::milli>;
        return std::chrono::duration_cast<D>(d).count();
    };
    std::stringstream fmt;
    fmt << std::fixed << std::setprecision(1)
        << " cur: " << this->sec_last
        << " avg: " << this->avg
        << " min: " << cast(this->min_dt)
        << " max: " << cast(this->max_dt);
    return fmt.str();
}

}
