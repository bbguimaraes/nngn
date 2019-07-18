#!/bin/bash
set -euo pipefail

pkgs=(
    # base
    base-devel clang gdb glfw-x11 qt5-base
    # emscripten
    emscripten
    # lua
    lua
    # tools
    qt5-charts rust
    # gl
    glew
    # vk
    shaderc vulkan-headers vulkan-icd-loader vulkan-intel
    vulkan-validation-layers xxd
    # cl
    clinfo intel-compute-runtime ocl-icd opencl-headers
    # font
    ttf-dejavu
)
root=build/root
pacstrap -cd "$root/" --needed "${pkgs[@]}"
systemd-nspawn -D "$root/" --console pipe sh <<EOF
set -eu
getent group  nngn > /dev/null || groupadd --gid 100 nngn
getent passwd nngn > /dev/null || useradd --uid 1000 --gid nngn nngn
EOF
systemd-nspawn -D "$root/" \
    --bind "$PWD:/home/nngn" --bind /dev/dri --bind /dev/input \
    --user nngn --setenv DISPLAY=:0
