#ifndef NNGN_TIMING_SCHEDULE_H
#define NNGN_TIMING_SCHEDULE_H

#include <chrono>
#include <vector>

#include "utils/def.h"

#include "timing.h"

namespace nngn {

/**
 * Executor of deferred and recurrent tasks.
 *
 * Tasks can be scheduled on a given frame (\ref next, \ref frame), at a
 * given time (\ref in, \ref at), or a combination of those (execution happens
 * at the first occurrence of either).  A separate category exists for tasks
 * that should be executed when the application terminates (\ref atexit).
 */
class Schedule {
public:
    /**
     * Signature for task functions.
     * The single parameter is a pointer to the data associated with the task on
     * construction.  \c true should be returned on success.
     */
    using Fn = bool(*)(void*);
    enum Flag : u8 {
        NONE,
        /** Whether a failure in the task is critical or can be tolerated. */
        IGNORE_FAILURES = 1u << 0,
        /** After it is triggered, execute again at every frame. */
        HEARTBEAT = 1u << 1,
    };
    struct Entry {
        /** The task's main function. */
        Fn f = {};
        /** Destructor for the associated data. */
        Fn dest = {};
        /** Opaque byte array associated with the task. */
        std::vector<std::byte> data = {};
        Flag flags = {};
    };
    void init(const Timing *t) { this->timing = t; }
    // Information
    std::size_t n(void) const { return this->v.size(); }
    std::size_t n_atexit(void) const { return this->atexit_v.size(); }
    // Scheduling functions
    std::size_t next(Entry e);
    std::size_t frame(u64 f, Entry e);
    std::size_t in(std::chrono::milliseconds t, Entry e);
    std::size_t at(Timing::time_point t, Entry e);
    std::size_t atexit(Entry e);
    // Cancelling
    bool cancel(std::size_t i);
    bool cancel_atexit(std::size_t i);
    // Update
    bool update(void);
    bool exit(void);
private:
    struct BaseEntry : Entry {
        bool active() const { return this->f; }
        bool is_heartbeat() const { return this->flags & Flag::HEARTBEAT; }
        bool call(void);
        bool destroy(void);
    };
    struct TimeEntry : BaseEntry {
        Timing::time_point time = {};
        u64 frame = 0;
        u32 gen = {};
        bool active(u32 cur_gen, u64 cur_frame, Timing::time_point now) const;
    };
    template<typename T>
    std::size_t add(std::vector<T> *v, T t);
    template<typename T>
    bool cancel_common(std::vector<T> *v, std::size_t i);
    std::vector<TimeEntry> v = {};
    std::vector<BaseEntry> atexit_v = {};
    u32 cur_gen = 0;
    const Timing *timing = nullptr;
};

}

#endif
