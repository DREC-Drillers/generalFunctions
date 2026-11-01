{
  lib,
  stdenv,
  cmake,
  ninja,
  gtest,
  clang-tools,
  buildLexer ? true,
  buildGeometry ? true,
  buildFiles ? false,
}:

stdenv.mkDerivation {
  pname = "general-functions";
  version = "0.1.0";

  # Local builds must not copy generated build trees back into the source.
  src = lib.cleanSourceWith {
    src = ../.;
    filter = path: type:
      let
        name = baseNameOf path;
        isBuildArtifact =
          name == "build" || name == "result" || lib.hasPrefix "result-" name;
      in
      lib.cleanSourceFilter path type && !isBuildArtifact;
  };

  nativeBuildInputs = [ cmake ninja ] ++ lib.optional buildGeometry clang-tools;

  buildInputs = [ gtest ];

  cmakeFlags = [
    (lib.cmakeBool "GENERAL_FUNCTIONS_BUILD_LEXER" buildLexer)
    (lib.cmakeBool "GENERAL_FUNCTIONS_BUILD_GEOMETRY" buildGeometry)
    (lib.cmakeBool "GENERAL_FUNCTIONS_BUILD_FILES" buildFiles)
    (lib.cmakeBool "GENERAL_FUNCTIONS_BUILD_TESTS" (buildLexer || buildGeometry))
  ] ++ lib.optional (buildGeometry && stdenv.cc.isClang)
    # CMake otherwise finds the unwrapped scanner, which misses Nix header paths.
    "-DCMAKE_CXX_COMPILER_CLANG_SCAN_DEPS=${clang-tools}/bin/clang-scan-deps";

  doCheck = buildLexer || buildGeometry;
  checkPhase = ''
    runHook preCheck
    ctest --output-on-failure --no-tests=error
    runHook postCheck
  '';

  meta.description = "Selectable C++ lexer, geometry and file utility components";
}

