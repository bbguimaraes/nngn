{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = [
    pkgs.pkg-config
  ];
  buildInputs = [
    pkgs.ccache
    pkgs.clang_12
    pkgs.gcc11
    pkgs.glew
    pkgs.glfw
    pkgs.glslang
    pkgs.lua5_4
    pkgs.qt5.qtbase
    pkgs.qt5.qtcharts
    pkgs.vulkan-headers
    pkgs.vulkan-loader
  ];
}
