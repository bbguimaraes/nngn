{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.freetype
    pkgs.gcc10
    pkgs.glew
    pkgs.glfw
    pkgs.libpng
    pkgs.lua5_3
    pkgs.ocl-icd
    pkgs.opencl-headers
    pkgs.qt5.qtbase
    pkgs.qt5.qtcharts
    pkgs.vulkan-headers
    pkgs.vulkan-loader
  ];
}
