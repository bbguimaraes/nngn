#ifndef NNGN_FPS_H
#define NNGN_FPS_H

#include <chrono>
#include <queue>
#include <string>

#include "timing.h"

namespace nngn {

struct FPS {
    using frame_queue = std::queue<Timing::clock::duration>;
    static constexpr frame_queue::size_type default_size = 60;
    Timing::clock::time_point last_f = {}, last_t = {};
    Timing::clock::duration min_dt = {}, max_dt = {};
    Timing::clock::duration avg_sum = {};
    frame_queue avg_hist;
    float avg = 0;
    size_t sec_count = 0, sec_last = 0;
    FPS() : FPS(default_size) {}
    explicit FPS(frame_queue::size_type n);
    void init(Timing::clock::time_point t);
    void frame(Timing::clock::time_point t);
    void reset_min_max();
    std::string to_string() const;
};

}

#endif
