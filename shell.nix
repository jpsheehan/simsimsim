{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [
        SDL2
        SDL2_ttf
        SDL2_image
        gcc
        pkgconfig
        valgrind
        gdb
        gnumake
    ];
    shellHook = ''
    '';
}
