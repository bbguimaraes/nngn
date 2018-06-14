#include "adhoc.h"

#include <numeric>

#include "utils/log.h"

namespace nngn {

static void end_step(AdHocTimer::Step *s, Timing::time_point *last) {
    const auto now = Timing::clock::now();
    s->t = now - std::exchange(*last, now);
}

void AdHocTimer::add(std::string_view name) {
    this->end();
    this->steps.push_back(Step{name, Timing::duration{}});
}

void AdHocTimer::end() {
    if(!this->steps.empty())
        end_step(&this->steps.back(), &this->last);
}

void AdHocTimer::log() {
    using D = std::chrono::duration<float, std::milli>;
    auto total = Timing::duration{};
    for(const auto &x : this->steps) {
        total += x.t;
        Log::l()
            << x.name << ": "
            << std::chrono::duration_cast<D>(x.t).count() << "ms\n";
    }
    Log::l()
        << "total: "
        << std::chrono::duration_cast<D>(total).count()
        << "ms\n";
}

}
