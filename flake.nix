{
  description = "Nix";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs =
    {
      self,
      nixpkgs,
      ...
    }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = pkgs.mkShell rec {
        packages = with pkgs; [
          gcc
          binutils
        ];

        buildInputs = with pkgs; [
          # pkg-config

          # glibc
          # libclang
          # libv4l
          # linuxHeaders

          # fontconfig
          # freetype
          # freetype.dev
          # libGL
          # pkg-config
          # libx11
          # libxcursor
          # libxi
          # libxrandr
          # wayland
          # libxkbcommon

          cmake

          libGL
          libglvnd
          libGLU

          glew
          glfw
          glm
          assimp
          zlib
        ];

        nativeBuildInputs = buildInputs;

        LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath buildInputs;
        LIBRARY_PATH = pkgs.lib.makeIncludePath buildInputs;
        CPATH = pkgs.lib.makeIncludePath buildInputs;
        CMAKE_PREFIX_PATH = pkgs.lib.makeIncludePath buildInputs;
      };
    };

}
