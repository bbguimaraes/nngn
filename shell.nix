{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = [
    pkgs.pkg-config
  ];
  buildInputs = [
    pkgs.ccache
    pkgs.clang_12
    pkgs.freetype
    pkgs.gcc11
    pkgs.glew
    pkgs.glfw
    pkgs.glslang
    pkgs.libpng
    pkgs.lua5_4
    pkgs.ocl-icd
    pkgs.opencl-headers
    pkgs.qt5.qtbase
    pkgs.qt5.qtcharts
    pkgs.vulkan-headers
    pkgs.vulkan-loader
  ];
}
