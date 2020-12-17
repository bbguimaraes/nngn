#include "frame_buffer.h"

namespace {

using Flag = nngn::Graphics::TerminalFlag;

void write_prefix(std::span<char> s, nngn::Flags<Flag> f) {
    using C = nngn::VT100EscapeCode;
    [[maybe_unused]] void *p = s.data();
    if(f.is_set(Flag::CLEAR))
        p = nngn::memcpy(p, C::clear);
    if(f.is_set(Flag::REPOSITION))
        p = nngn::memcpy(p, C::pos);
    assert(p == s.data() + s.size());
}

void resize_and_fill(
    nngn::term::FrameBuffer *f, std::vector<char> *v,
    std::size_t n, auto fill)
{
    constexpr auto size = sizeof(fill);
    const auto old = v->size();
    if constexpr(size == 1) {
        if(!v->empty()) {
            const auto pixels = f->pixels();
            std::fill(begin(pixels), end(pixels), fill);
        }
        if(old != n)
            v->resize(n, fill);
    } else {
        if(old != n)
            v->resize(n);
        assert(!(f->pixels().size() % size));
        nngn::fill_with_pattern(
            nngn::owning_view{nngn::as_byte_span(&fill)},
            nngn::byte_cast<std::byte>(f->pixels()));
    }
}

}

namespace nngn::term {

void FrameBuffer::resize_and_clear(uvec2 s) {
    this->m_size = s;
    this->flip_tmp.resize(s.x);
    const auto empty = this->v.empty();
    auto n = this->prefix_size
        + nngn::Math::product(this->m_size);
    resize_and_fill(this, &this->v, n, ' ');
    if(!empty)
        return;
    write_prefix(this->prefix(), this->flags);
}

void FrameBuffer::flip(void) {
    const auto [w, h] = this->m_size;
    const auto p = this->pixels();
    auto *const tmp = this->flip_tmp.data();
    for(std::size_t y = 0, e = h / 2; y < e; ++y) {
        auto *const p0 = &p[w * y];
        auto *const p1 = &p[w * (h - y - 1)];
        std::copy(p0, p0 + w, tmp);
        std::copy(p1, p1 + w, p0);
        std::copy(tmp, tmp + w, p1);
    }
}

}
