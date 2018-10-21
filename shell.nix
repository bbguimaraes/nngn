{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.gcc10
    pkgs.glew
    pkgs.glfw
    pkgs.lua5_3
    pkgs.qt5.qtbase
    pkgs.qt5.qtcharts
  ];
}
