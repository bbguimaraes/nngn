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
        base-devel clang emscripten gdb glfw-x11 qt5-base
USER nngn
