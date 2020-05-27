#include <algorithm>
#include <cassert>

#include "utils/log.h"

#include "profile.h"
#include "schedule.h"

namespace {

/** Applies \c f to all elements of \c v, \c f may modify \c v. */
template<typename T, typename F>
bool mut_iter(T *v, F &&f) {
    for(std::size_t i = 0; i < v->size(); ++i)
        if(!f((*v)[i]))
            return false;
    return true;
}

template<typename ...Ts>
bool exec(std::ranges::forward_range auto *r, Ts ...ts) {
    return mut_iter(r, [...ts = std::move(ts)](auto &x) {
        return !x.active(ts...)
            || x.call()
            || (x.flags & nngn::Schedule::Flag::IGNORE_FAILURES);
    });
}

}

namespace nngn {

bool Schedule::BaseEntry::call(void) {
    return this->f(static_cast<void*>(this->data.data()));
}

bool Schedule::BaseEntry::destroy(void) {
    assert(this->f);
    if(this->dest && !this->dest(this->data.data()))
        return false;
    *this = {};
    return true;
}

bool Schedule::TimeEntry::active(
    u32 gen_, u64 cur_frame, Timing::time_point now
) const {
    return BaseEntry::active()
        && this->gen < gen_
        && this->time <= now
        && this->frame <= cur_frame;
}

template<typename T>
std::size_t Schedule::add(std::vector<T> *v_, T t) {
    const auto e = v_->end();
    auto it = std::find_if_not(begin(*v_), e, std::mem_fn(&BaseEntry::active));
    if(it == e)
        it = v_->insert(it, t);
    else
        *it = t;
    return static_cast<std::size_t>(std::distance(begin(*v_), it));
}

template<typename T>
bool Schedule::cancel_common(std::vector<T> *v_, std::size_t i) {
    assert(i < v_->size());
    auto it = begin(*v_) + static_cast<std::ptrdiff_t>(i);
    if(!it->destroy())
        return false;
    const auto e = end(*v_);
    if(std::find_if(it, e, std::mem_fn(&BaseEntry::active)) == e)
        v_->erase(it, e);
    return true;
}

std::size_t Schedule::next(Entry e) {
    TimeEntry te = {{std::move(e)}};
    te.gen = this->cur_gen;
    return this->add(&this->v, std::move(te));
}

std::size_t Schedule::in(std::chrono::milliseconds t, Entry e) {
    return this->at(t + this->timing->now, std::move(e));
}

std::size_t Schedule::at(Timing::time_point t, Entry e) {
    TimeEntry te = {{std::move(e)}};
    te.time = t;
    te.gen = this->cur_gen;
    return this->add(&this->v, std::move(te));
}

std::size_t Schedule::frame(u64 f, Entry e) {
    TimeEntry te = {{std::move(e)}};
    te.frame = f;
    te.gen = this->cur_gen;
    return this->add(&this->v, std::move(te));
}

std::size_t Schedule::atexit(Entry e) {
    return this->add(&this->atexit_v, {std::move(e)});
}

bool Schedule::cancel(std::size_t i) {
    return this->cancel_common(&this->v, i);
}

bool Schedule::cancel_atexit(std::size_t i) {
    return this->cancel_common(&this->atexit_v, i);
}

bool Schedule::update(void) {
    NNGN_LOG_CONTEXT_CF(Schedule);
    NNGN_PROFILE_CONTEXT(schedule);
    const auto now = this->timing->now;
    const auto cur_frame = this->timing->frame;
    const auto g = ++this->cur_gen;
    if(!exec(&this->v, g, cur_frame, now))
        return false;
    for(auto &x : this->v)
        if(x.active(g, cur_frame, now) && !x.is_heartbeat() && !x.destroy())
            return false;
    this->v.erase(
        std::find_if(
            rbegin(this->v), rend(this->v),
            std::mem_fn(&nngn::Schedule::BaseEntry::active)).base(),
        end(this->v));
    return true;
}

bool Schedule::exit(void) {
    NNGN_LOG_CONTEXT_CF(Schedule);
    if(!exec(&this->atexit_v))
        return false;
    const auto ret = std::ranges::all_of(this->atexit_v, &BaseEntry::destroy);
    this->atexit_v.clear();
    return ret;
}

}
