#!/bin/bash
set -euo pipefail

LUA_MAJ=5
LUA_MIN=4
LUA_VERSION=$LUA_MAJ.$LUA_MIN.4
LUA_DLL=lua${LUA_MAJ}${LUA_MIN}.dll
LUA_PKG=https://www.lua.org/ftp/lua-$LUA_VERSION.tar.gz

main() {
    [[ "$#" -eq 0 ]] && usage
    local cmd=$1; shift
    case "$cmd" in
    mingw)
        [[ "$#" -eq 0 ]] && usage
        local dir=$1/mingw
        build_lua "$dir" 'make mingw' "$LUA_DLL"
        ln --symbolic --force --no-dereference "$LUA_DLL" "$dir/lib/lua.dll";;
    emscripten)
        [[ "$#" -eq 0 ]] && usage
        local dir=$1/emscripten
        local script_dir=$(dirname "$(readlink -f "$BASH_SOURCE")")
        build_lua \
            "$dir" 'emmake make generic' liblua.a \
            "$script_dir/emscripten/patches/lua.patch";;
    *) usage;;
    esac
}

usage() {
    cat >&2 <<EOF
Usage: $0 mingw|emscripten BUILD_DIR [MAKE_CMD] [LIB]
EOF
    return 1
}

build_lua() {
    local dir=$1 make=$2 lib=$3 x; shift 3
    mkdir --parents "$dir/include" "$dir/lib" "$dir/src"
    pushd "$dir/src" > /dev/null
    [ -e "lua-$LUA_VERSION.tar.gz" ] || curl -LO "$LUA_PKG"
    [ -e "lua-$LUA_VERSION" ] || tar -xf "lua-$LUA_VERSION.tar.gz"
    pushd "lua-$LUA_VERSION" > /dev/null
    for x; do
        patch -Np0 < "$x"
    done
    pushd src/
    $make ALL="$lib" ${CC+CC="$CC"}
    cp -at "$dir/include" lua.h luaconf.h lualib.h lauxlib.h lua.hpp
    cp -at "$dir/lib" "$lib"
    popd > /dev/null
    popd > /dev/null
    popd > /dev/null
}

main "$@"
