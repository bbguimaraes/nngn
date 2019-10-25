#include "obj.h"

#include <charconv>

#include "utils/ranges.h"

namespace nngn::detail {

bool obj_invalid(std::size_t i, std::string_view line) {
    Log::l()
        << "line " << i << ": invalid command: "
        << std::quoted(line) << '\n';
    return false;
}

std::string_view obj_parse_line(const char *p, std::streamsize n) {
    std::string_view ret = {p, static_cast<std::size_t>(n)};
    if(ret.ends_with('\0'))
        ret.remove_suffix(1);
    if(ret.ends_with('\r'))
        ret.remove_suffix(1);
    const auto e = std::find_if(
        std::make_reverse_iterator(std::ranges::find(ret, '#')),
        rend(ret),
        [](auto c) { return c != ' '; });
    return std::string_view{begin(ret), e.base()};
}

bool obj_read_vec(
    std::size_t line, std::string_view args, const char *name,
    vec3 *v)
{
    // TODO use <charconv> when floating-point input is supported
    std::stringstream s = {};
    s.write(args.data(), static_cast<std::streamsize>(args.size()));
    for(auto &x : *v) {
        if(s >> x)
            continue;
        if(s.eof())
            break;
        Log::l()
            << "line " << line << ": invalid " << name << ": "
            << std::quoted(args) << '\n';
        return false;
    }
    return true;
}

bool obj_read_face_indices(
    std::size_t line, std::string_view args, std::string_view *ps, zvec3 *pv)
{
    std::string_view s = *ps;
    const auto read = [&s, e = data_end(s)](std::size_t *p) {
        const auto b = std::ranges::find_if(s, [](char c) { return c != ' '; });
        const auto res = std::from_chars(&*b, e, *p);
        s = {res.ptr, e};
        return !std::error_condition{res.ec};
    };
    auto &v = *pv;
    if(s.empty() || s[0] != ' ' || !read(&v[0]))
        goto err;
    if(s.empty() || s[0] == ' ') {
        v[1] = v[2] = v[0];
        goto ok;
    }
    if(s[0] != '/')
        goto err;
    s.remove_prefix(1);
    if(s.empty())
        goto err;
    if(s[0] == '/')
        v[1] = 1;
    else if(!read(&v[1]))
        goto err;
    if(s.empty() || s[0] == ' ') {
        v[2] = 1;
        goto ok;
    }
    if(s[0] != '/')
        goto err;
    s.remove_prefix(1);
    if(!read(&v[2]))
        goto err;
ok:
    *ps = s;
    return true;
err:
    Log::l()
        << "line " << line << ": invalid face: "
        << std::quoted(args) << '\n';
    return false;
}

}
