#ifndef NNGN_SUN_H
#define NNGN_SUN_H

#include <chrono>

#include "math/vec3.h"

namespace nngn {

class Sun {
public:
    using duration = std::chrono::milliseconds;
private:
    float m_incidence = 0, m_sin = 0, m_cos = 0, m_arc_len = 0;
    duration m_time = {};
    bool m_updated = false;
public:
    Sun();
    float incidence() const { return this->m_incidence; }
    duration time() const { return this->m_time; }
    auto time_ms() const { return this->m_time.count(); }
    bool updated(void) const { return this->m_updated; }
    void set_incidence(float a);
    void set_time(duration t);
    void set_time_ms(unsigned t) { return this->set_time(Sun::duration(t)); }
    void set_updated(bool b) { this->m_updated = b; }
    vec3 dir() const;
};

}

#endif
