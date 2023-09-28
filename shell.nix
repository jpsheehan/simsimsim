{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [
        SDL2
        SDL2_ttf
        gcc
        pkgconfig
        valgrind
        gdb
        gnumake
    ];
    shellHook = ''
    '';
}
