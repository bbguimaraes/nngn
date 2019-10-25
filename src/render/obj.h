#ifndef NNGN_RENDER_OBJ_H
#define NNGN_RENDER_OBJ_H

#include <concepts>
#include <iomanip>
#include <istream>
#include <ranges>
#include <sstream>

#include "math/vec3.h"
#include "utils/log.h"
#include "utils/span.h"

namespace nngn {

namespace detail {

bool obj_invalid(std::size_t i, std::string_view line);
std::string_view obj_parse_line(const char *p, std::streamsize n);
bool obj_read_vec(
    std::size_t line, std::string_view args, const char *name, vec3 *v);
bool obj_read_face_indices(
    std::size_t line, std::string_view args,
    std::string_view *ps, zvec3 *pv);

}

/**
  * Processes an OBJ file using the callable arguments.
  * Each argument is called immediately after a line containing the
  * corresponding command is found.
  *
  * For vertices, texture coordinates, and normal vectors, missing elements
  * result in zeroes in the value passed to the callable (e.g. `v 1` results in
  * a call to `vertex({1, 0, 0})`).
  *
  * Faces are always emitted as triangles.  After being processed as defined by
  * the OBJ format (i.e. filling values for entries such as `1`, `1/2`, `1//2`,
  * etc.), each group of three vertices is passed as an argument to the `face`
  * callable.  Currently, the list is treated as a triangular fan, such that a
  * line in the form `f 1 2 3 4 5 …` results in the triangles `{1, 2, 3}`, `{1,
  * 3, 4}`, `{1, 4, 5}`, etc.  This list of triangular faces is assumed to be in
  * counter-clockwise (i.e. right-handed) orientation.
  *
  * \param vertex Called for each line containing a `v` definition.
  * \param tex Called for each line containing a `vt` definition.
  * \param normal Called for each line containing a `vn` definition.
  * \param face
  *     Called repeatedly for each line containing an `f` definition.  Each
  *     element in the array argument corresponds to a `{v,vt,vn}` group and the
  *     array defines a triangular face, as described in detail above.
  */
bool parse_obj(
    std::istream *f,
    std::span<char> buffer,
    std::invocable<vec3> auto &&vertex,
    std::invocable<vec3> auto &&tex,
    std::invocable<vec3> auto &&normal,
    // TODO negative indices?
    std::invocable<std::array<zvec3, 3>> auto &&face);

inline bool parse_obj(
    std::istream *f,
    std::span<char> buffer,
    std::invocable<vec3> auto &&vertex,
    std::invocable<vec3> auto &&tex,
    std::invocable<vec3> auto &&normal,
    std::invocable<std::array<zvec3, 3>> auto &&face)
{
    NNGN_LOG_CONTEXT_F();
    constexpr auto read_vec = [](
        std::size_t line, std::string_view args, const char *name, auto &&fn)
    {
        vec3 v = {};
        return detail::obj_read_vec(line, args, name, &v)
            && (FWD(fn)(v), true);
    };
    constexpr auto read_face = [](
        std::size_t line, std::string_view args, auto &&fn)
    {
        constexpr auto read = detail::obj_read_face_indices;
        constexpr auto dec = [](auto *v) { for(auto &x : *v) --x; };
        std::string_view s = args;
        zvec3 f0 = {}, f1 = {}, f2 = {};
        if(!(
            read(line, args, &s, &f0)
            && read(line, args, &s, &f1)
            && read(line, args, &s, &f2)
        ))
            return false;
        dec(&f0), dec(&f1), dec(&f2);
        FWD(fn)(std::array{f0, f1, f2});
        while(!s.empty()) {
            if(!read(line, args, &s, &f1))
                return false;
            dec(&f1);
            std::swap(f1, f2);
            FWD(fn)(std::array{f0, f1, f2});
        }
        return true;
    };
    using namespace std::string_view_literals;
    constexpr std::array ignored = {
        "g"sv, "mtl"sv, "mtllib"sv, "o"sv, "s"sv, "usemtl"sv,
    };
    static_assert(std::ranges::is_sorted(ignored));
    const auto max = static_cast<std::streamsize>(buffer.size());
    for(std::size_t line = 1;; ++line) {
        if(!f->getline(buffer.data(), max)) {
            if(f->eof())
                return true;
            Log::l() << "line " << line << " too long (max: " << max << ")\n";
            return false;
        }
        const auto s = detail::obj_parse_line(buffer.data(), f->gcount());
        if(s.empty())
            continue;
        const auto [cmd, args] = split(s, std::ranges::find(s, ' '));
        if(cmd == "v") {
            if(!read_vec(line, args, "vertex", FWD(vertex)))
                return false;
        } else if(cmd == "vt") {
            if(!read_vec(line, args, "texture coordinates", FWD(tex)))
                return false;
        } else if(cmd == "vn") {
            if(!read_vec(line, args, "vertex normal", FWD(normal)))
                return false;
        } else if(cmd == "f") {
            if(!read_face(line, args, FWD(face)))
                return false;
        } else if(!std::ranges::binary_search(ignored, cmd))
            return detail::obj_invalid(line, s);
    }
}

}

#endif
