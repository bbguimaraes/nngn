{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = [
    pkgs.pkg-config
  ];
  buildInputs = [
    pkgs.gcc11
    pkgs.glfw
    pkgs.lua5_4
    pkgs.qt5.qtbase
    pkgs.qt5.qtcharts
  ];
}
