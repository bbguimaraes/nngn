#!/bin/bash
set -euo pipefail

SCRIPTS_DIR=$PWD/scripts
PATCHES_DIR=$SCRIPTS_DIR/emscripten/patches
LUA_MAJ=5
LUA_MIN=4
LUA_VERSION=$LUA_MAJ.$LUA_MIN.1
LUA_DLL=lua${LUA_MAJ}${LUA_MIN}.dll
LUA_PKG=https://www.lua.org/ftp/lua-$LUA_VERSION.tar.gz

main() {
    local arg= build_dir
    [[ "$#" -gt 0 ]] && { arg=$1; shift; }
    case "$arg" in
    mingw)
        [[ "$#" -gt 0 ]] || { usage >&2; return 1; }
        build_dir=$1/mingw
        mkdir --parents "$build_dir/include" "$build_dir/lib" "$build_dir/src"
        build_lua "$build_dir" 'make mingw' "$LUA_DLL"
        build_sol "$build_dir"
        ln --symbolic --force --no-dereference \
            "$LUA_DLL" "$build_dir/lib/lua.dll";;
    emscripten)
        [[ "$#" -gt 0 ]] || { usage >&2; return 1; }
        build_dir=$1/emscripten
        mkdir --parents "$build_dir/include" "$build_dir/lib" "$build_dir/src"
        build_lua \
            "$build_dir" 'emmake make generic' liblua.a \
            "$PATCHES_DIR/lua.patch"
        build_sol "$build_dir";;
    *) usage >&2; return 1;;
    esac
}

usage() {
    echo "Usage: $0 mingw|emscripten <build_dir>"
}

build_lua() {
    local build_dir=$1 make=$2 lib=$3; shift 3
    pushd "$build_dir/src" > /dev/null
    [ -e "lua-$LUA_VERSION.tar.gz" ] || curl -LO "$LUA_PKG"
    [ -e "lua-$LUA_VERSION" ] || tar -xf "lua-$LUA_VERSION.tar.gz"
    pushd "lua-$LUA_VERSION" > /dev/null
    for x; do
        patch -Np0 < "$x"
    done
    pushd src/
    $make ALL="$lib" ${CC+CC="$CC"}
    cp -at "$build_dir/include" lua.h luaconf.h lualib.h lauxlib.h lua.hpp
    cp -at "$build_dir/lib" "$lib"
    popd > /dev/null
    popd > /dev/null
    popd > /dev/null
}

build_sol() {
    local build_dir=$1
    ln -s /usr/include/sol "$build_dir/include/sol"
}

main "$@"
