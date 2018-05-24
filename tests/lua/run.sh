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
