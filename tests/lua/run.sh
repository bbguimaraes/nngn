#!/bin/bash
set -euo pipefail

nngn=$PWD/nngn

run() {
    echo "$@"
    "$nngn" "@$@"
}

[[ "${srcdir:-}" ]] && cd "$srcdir"
lua tests/lua/unit.lua
run tests/lua/input.lua
run tests/lua/entity.lua
run tests/lua/camera.lua
run tests/lua/player.lua
run tests/lua/font.lua
