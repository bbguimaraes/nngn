#ifndef NNGN_TIMING_LIMIT_H
#define NNGN_TIMING_LIMIT_H

#include "timing.h"

namespace nngn {

/**
 * Simulates a v-sync pause using `sleep`.
 *
 * \ref limit should be called once at the end of each frame.  A constant rate
 * of 60FPS, scaled by the inverse of \ref interval, is maintained no matter the
 * duration between each call to \ref limit.
 *
 * If one cycle takes too long, \ref limit returns immediately a number of times
 * necessary to generate the correct amount of frames as if the rate had been
 * maintained (i.e. frames are not skipped).
 */
class FrameLimiter {
public:
    int interval(void) const { return this->m_interval; }
    /** \see Graphics::set_swap_interval */
    void set_interval(int i) { this->m_interval = i; }
    /** Sleeps for as long as necessary to maintain a constant frame rate. */
    void limit(void);
private:
    using time_point = nngn::Timing::time_point;
    using clock = nngn::Timing::clock;
    using duration = nngn::Timing::duration;
    int m_interval = 1;
    time_point last = clock::now();
};

}

#endif
