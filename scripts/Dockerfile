# podman run --rm -it \
#     --user 1000 \
#     -v /dev/dri:/dev/dri \
#     -v /dev/input:/dev/input \
#     -v /tmp/.X11-unix:/tmp/.X11-unix \
#     -v "$PWD:/tmp/nngn:z" \
#     -e DISPLAY="$DISPLAY" \
#     -w /tmp/nngn nngn
FROM archlinux/base
RUN groupadd --gid 100 nngn \
    && useradd --uid 1000 --gid nngn nngn \
    && pacman --noconfirm -Syu \
        base-devel clang emscripten gdb glew glfw-x11 lua qt5-base qt5-charts \
        rust shaderc vulkan-headers vulkan-icd-loader vulkan-intel \
        vulkan-validation-layers xxd
USER nngn
