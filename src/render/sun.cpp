#include <cmath>

#include "math/math.h"

#include "sun.h"

namespace nngn {

Sun::Sun() { this->set_incidence(0); }

void Sun::set_incidence(float a) {
    this->m_incidence = a;
    this->m_sin = std::sin(a);
    this->m_cos = std::cos(a);
    this->m_updated = true;
}

void Sun::set_time(duration t) {
    this->m_time = t;
    this->m_arc_len = t * Math::pi<float>() / std::chrono::hours(12);
    this->m_updated = true;
}

vec3 Sun::dir() const {
    const float sin = std::sin(this->m_arc_len);
    const float cos = std::cos(this->m_arc_len);
    return {-sin, -cos * this->m_cos, -cos * this->m_sin};
}

}
