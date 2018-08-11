#include <algorithm>

#include <lua.hpp>

#include "entity.h"

#include "timing/timing.h"
#include "utils/log.h"

#include "light.h"

namespace nngn {

void Light::set_pos(const vec3 &p)
    { this->pos = p; this->updated = true; }
void Light::set_dir(const vec3 &p)
    { this->dir = p; this->updated = true; }
void Light::set_color(const vec4 &c)
    { this->color = c; this->updated = true; }
void Light::set_spec(float v)
    { this->spec = v; this->updated = true; }
void Light::set_cutoff(float v)
    { this->cutoff = v; this->updated = true; }

void Light::set_att(lua_State *L) {
    this->att = lua_gettop(L) == 2
        ? Light::att_for_range(static_cast<float>(lua_tonumber(L, 2)))
        : vec3{
            static_cast<float>(lua_tonumber(L, 2)),
            static_cast<float>(lua_tonumber(L, 3)),
            static_cast<float>(lua_tonumber(L, 4))};
    this->updated = true;
}

vec3 Light::ortho_view_pos() const {
    const auto p = this->dir * -512.0f;
    return {p.x, -p.y, p.z};
}

void Light::write_to_ubo_dir(LightsUBO *ubo, size_t i) const {
    ubo->dir.dir[i] = {this->dir, 0};
    ubo->dir.color_spec[i] = {this->color.xyz() * this->color.w, this->spec};
}

void Light::write_to_ubo_point(LightsUBO *ubo, size_t i, bool zsprites) const {
    const auto fz = static_cast<float>(zsprites);
    ubo->point.color_spec[i] = {this->color.xyz() * this->color.w, this->spec};
    ubo->point.pos[i] =
        {this->pos.x, this->pos.y + fz * this->pos.z, this->pos.z, 0};
    ubo->point.dir[i] = {this->dir, 0};
    ubo->point.att_cutoff[i] = {this->att, this->cutoff};
}

void Lighting::set_enabled(bool b) {
    this->flags.set(Flag::ENABLED, b);
    this->flags |= Flag::UPDATED;
}

void Lighting::set_zsprites(bool b) {
    this->flags.set(Flag::ZSPRITES, b);
    this->flags |= Flag::UPDATED;
}

void Lighting::set_update_sun(bool b) {
    this->flags.set(Flag::UPDATE_SUN, b);
    this->flags |= Flag::UPDATED;
}

void Lighting::set_ambient_light(const vec4 &v) {
    this->m_ambient_light = v;
    this->flags |= Flag::UPDATED;
}

void Lighting::set_ambient_anim(LightAnimation a) {
    this->m_ambient_anim = a;
}

Light *Lighting::add_light(Light::Type t) {
    const auto add = [this, t](auto *v, auto *n, auto *s) -> Light* {
        if(*n == MAX_LIGHTS) {
            Log::l()
                << "Lighting::add_light: cannot add more " << s << " lights"
                << std::endl;
            return nullptr;
        }
        this->flags |= Flag::UPDATED;
        return &((*v)[(*n)++] = Light(t));
    };
    switch(t) {
    case Light::Type::DIR:
        return add(&this->m_dir_lights, &this->n_dir, "dir");
    case Light::Type::POINT:
        return add(&this->m_point_lights, &this->n_point, "point");
    }
    return nullptr;
}

void Lighting::remove_light(Light *l) {
    const auto f = [this, l](auto *v, size_t *n) {
        this->flags |= Flag::UPDATED;
        const auto i = static_cast<size_t>(l - v->data());
        assert(i < v->size());
        if(i == --(*n))
            return;
        *l = (*v)[*n];
        l->updated = true;
        if(l->e)
            l->e->light = l;
    };
    if(l->e)
        l->e->light = nullptr;
    switch(l->type) {
    case Light::Type::DIR:
        return f(&this->m_dir_lights, &this->n_dir);
    case Light::Type::POINT:
        return f(&this->m_point_lights, &this->n_point);
    }
}

bool Lighting::update(const Timing &t) {
    const auto update_ubo = [this]() {
        this->m_ubo.view_pos =
            {this->view_pos.x, -this->view_pos.y, this->view_pos.z};
        this->m_ubo.ambient =
            this->m_ambient_light.xyz() * this->m_ambient_light.w;
        this->m_ubo.n_dir = static_cast<uint32_t>(this->n_dir);
        this->m_ubo.n_point = static_cast<uint32_t>(this->n_point);
        for(size_t i = 0, n = this->n_dir; i < n; ++i)
            this->m_dir_lights[i].write_to_ubo_dir(&this->m_ubo, i);
        for(size_t i = 0, n = this->n_point; i < n; ++i)
            this->m_point_lights[i].write_to_ubo_point(
                &this->m_ubo, i, this->flags.is_set(Flag::ZSPRITES));
    };
    const auto any_updated = [](const auto &v, size_t n) {
        return std::any_of(
            v.cbegin(), v.cbegin() + n,
            [](auto &x) { return x.updated; });
    };
    const auto clear_updated = [](auto *v, size_t n) {
        std::for_each(
            v->begin(), v->begin() + n,
            [](auto &x) { x.updated = false; });
    };
    const bool enabled = this->flags.is_set(Flag::ENABLED);
    bool updated = this->flags.is_set(Flag::UPDATED) || this->m_sun.updated();
    if(!enabled) {
        if(!updated)
            return false;
        this->flags.clear(Flag::UPDATED);
        this->m_ubo = {};
        return true;
    }
    this->m_ambient_anim.update(
        t, this->math->rnd_generator(),
        &this->m_ambient_light.w, &updated);
    if(this->m_sun_light && this->flags.is_set(Flag::UPDATE_SUN))
        this->m_sun.set_time(this->m_sun.time()
            + std::chrono::duration_cast<Sun::duration>(t.dt * 3600));
    const bool view_updated =
        static_cast<bool>(this->flags & Flag::VIEW_UPDATED);
    const bool dir_updated = any_updated(this->m_dir_lights, this->n_dir);
    const bool point_updated = any_updated(this->m_point_lights, this->n_point);
    updated = updated || view_updated || dir_updated || point_updated
        || this->m_sun.updated();
    if(!updated)
        return false;
    this->flags.clear(Flag::UPDATED | Flag::VIEW_UPDATED);
    if(dir_updated)
        clear_updated(&this->m_dir_lights, this->n_dir);
    if(point_updated)
        clear_updated(&this->m_point_lights, this->n_point);
    this->m_sun.set_updated(false);
    if(this->m_sun_light)
        this->m_sun_light->set_dir(this->m_sun.dir());
    update_ubo();
    return true;
}

void Lighting::update_view(const vec3 &eye) {
    this->view_pos = eye;
    this->flags |= Flag::VIEW_UPDATED;
}

}
