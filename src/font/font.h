#ifndef NNGN_FONT_H
#define NNGN_FONT_H

#include <array>
#include <vector>

#include "math/vec2.h"
#include "utils/utils.h"

namespace nngn {

struct Graphics;

struct Font {
    static constexpr size_t N = 128;
    struct Character {
        uvec2 size;
        ivec2 bearing;
        float advance;
    };
    using Characters = std::array<Character, N>;
    unsigned int size = 0;
    Characters chars = {};
};

class Fonts {
    void *ft = {};
    std::vector<Font> v = {{}};
public:
    Graphics *graphics = nullptr;
    Fonts() = default;
    ~Fonts();
    NNGN_NO_COPY(Fonts)
    bool init();
    size_t n() const { return this->v.size(); }
    const Font *fonts() const { return this->v.data(); }
    uint32_t add(const Font &f);
    uint32_t load(unsigned int size, const char *filename);
};

}

#endif
