#!/bin/bash
set -euo pipefail

SCRIPTS_DIR=$PWD/scripts
PATCHES_DIR=$SCRIPTS_DIR/emscripten/patches
LUA_VERSION=5.4.1
LUA_PKG=https://www.lua.org/ftp/lua-$LUA_VERSION.tar.gz

main() {
    [[ "$#" -gt 0 ]] || { echo >&2 "Usage: $0 build_dir"; exit 1; }
    local build_dir=$1/emscripten; shift
    mkdir --parents "$build_dir/include" "$build_dir/lib" "$build_dir/src"
    build_lua "$build_dir"
    build_sol "$build_dir"
}

build_lua() {
    local build_dir=$1
    pushd "$build_dir/src" > /dev/null
    [ -e "lua-$LUA_VERSION.tar.gz" ] || curl -LO "$LUA_PKG"
    [ -e "lua-$LUA_VERSION" ] || tar -xf "lua-$LUA_VERSION.tar.gz"
    pushd "lua-$LUA_VERSION" > /dev/null
    patch -Np0 < "$PATCHES_DIR/lua.patch"
    pushd src/
    emmake make generic ALL=liblua.a
    cp -art "$build_dir/include" lua.h luaconf.h lualib.h lauxlib.h lua.hpp
    cp -at "$build_dir/lib" liblua.a
    popd > /dev/null
    popd > /dev/null
    popd > /dev/null
}

build_sol() {
    local build_dir=$1
    ln -s /usr/include/sol "$build_dir/include/sol"
}

main "$@"
