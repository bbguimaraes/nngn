#!/bin/bash
set -euo pipefail

main() {
    local type=$1; shift
    case "$type" in
    header) gen_header "$@";;
    *) gen_source "$type" "$@";;
    esac
}

gen_header() {
    cat <<EOF
#ifndef NNGN_GRAPHICS_SHADERS_H
#define NNGN_GRAPHICS_SHADERS_H

#include <cstdint>
#include <span>

namespace nngn {

$(gen_header_items "$@")

}

#endif
EOF
}

gen_source() {
    cat <<EOF
#include "shaders.h"

#include <array>

namespace nngn {

$(gen_source_items "$@")

}
EOF
}

gen_header_items() {
    local f dir name size
    for f; do
        name=$(var_name "$f")
        printf 'extern const std::span<const std::uint8_t> %s;\n' "$name"
    done
}

gen_source_items() {
    local type=$1; shift
    local f dir size name data
    for f; do
        [[ "$type" == vk ]] && truncate "$f" --size %4
        name=$(var_name "$f")
        data=${name}_DATA
        echo "static constexpr auto $data = std::to_array<std::uint8_t>({"
        xxd --include < "$f"
        echo '});'
        echo "const std::span<const std::uint8_t> $name = $data;"
    done
}

var_name() {
    local f=$1 type name
    type=$(basename "$(dirname "$f")")
    name=$(basename "$f")
    [[ "$type" == vk ]] && name=${name%.spv}
    name=${name//./_}
    printf GLSL_%s_%s "${type^^}" "${name^^}"
}

main "$@"
