{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = [
    pkgs.pkg-config
  ];
  buildInputs = [
    pkgs.ccache
    pkgs.clang_12
    pkgs.gcc11
    pkgs.glfw
    pkgs.qt5.qtbase
  ];
}
