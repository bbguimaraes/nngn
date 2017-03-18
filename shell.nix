{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.gcc10
    pkgs.glfw
    pkgs.qt5.qtbase
  ];
}
