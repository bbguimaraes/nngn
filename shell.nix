{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = [
    pkgs.pkg-config
  ];
  buildInputs = [
    pkgs.gcc11
    pkgs.glfw
    pkgs.qt5.qtbase
  ];
}
