{
  description = "GeneralFunctions: selectable C++ components";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs = { self, nixpkgs }:
    let
      # Only this platform is currently tested; do not imply broader support.
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      llvm = pkgs.llvmPackages_18;
      clangScanDeps = "${llvm.clang-tools}/bin/clang-scan-deps";

      buildComponentPackage = options:
        pkgs.callPackage ./nix/package.nix ({
          stdenv = llvm.stdenv;
          clang-tools = llvm.clang-tools;
        } // options);

      componentPackages = {
        lexer = buildComponentPackage { buildGeometry = false; };
        geometry = buildComponentPackage { buildLexer = false; };
        default = buildComponentPackage { };
      } // pkgs.lib.optionalAttrs
        (builtins.pathExists ./file.cpp && builtins.pathExists ./file.h)
        {
          # This output exists only in revisions containing the file utilities.
          files = buildComponentPackage {
            buildLexer = false;
            buildGeometry = false;
            buildFiles = true;
          };
        };

      # Test the public installed package, not targets from the source tree.
      installedConsumerCheck = pkgs.runCommand "general-functions-installed-consumer" {
        nativeBuildInputs = [
          pkgs.cmake
          pkgs.ninja
          llvm.stdenv.cc
          llvm.clang-tools
        ];
        buildInputs = [ componentPackages.default ];
      } ''
        cmake -S ${self}/tests/consumer -B build -G Ninja \
          -DCMAKE_CXX_COMPILER=${llvm.stdenv.cc}/bin/clang++ \
          -DCMAKE_CXX_COMPILER_CLANG_SCAN_DEPS=${clangScanDeps} \
          -DTEST_GEOMETRY=ON
        cmake --build build
        ./build/consumer
        ./build/geometry_consumer
        touch "$out"
      '';
    in {
      packages.${system} = componentPackages;

      checks.${system} = componentPackages // {
        installed-consumer = installedConsumerCheck;
      };

      devShells.${system}.default = (pkgs.mkShell.override {
        stdenv = llvm.stdenv;
      }) {
        packages = [ pkgs.cmake pkgs.ninja pkgs.git llvm.clang-tools ];
        GENERAL_FUNCTIONS_CLANG_SCAN_DEPS = clangScanDeps;
      };
    };
}

