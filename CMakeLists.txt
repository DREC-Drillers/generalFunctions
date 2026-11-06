cmake_minimum_required(VERSION 3.24)
project(GeneralFunctions VERSION 0.1.0 LANGUAGES CXX)

# C++20 importers must be scanned even though lexer supports CMake 3.24.
if(POLICY CMP0155)
    cmake_policy(SET CMP0155 NEW)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(GENERAL_FUNCTIONS_BUILD_LEXER "Build the lexer component" ON)
option(GENERAL_FUNCTIONS_BUILD_GEOMETRY "Build the C++20 geometry module" ON)
option(GENERAL_FUNCTIONS_BUILD_FILES "Build file utilities (requires file.cpp/file.h)" OFF)
option(GENERAL_FUNCTIONS_BUILD_TREE "Build the header-only binary tree component" ON)
option(GENERAL_FUNCTIONS_BUILD_TESTS "Build GoogleTest component tests" OFF)

function(add_general_functions_component component_name cpp_standard)
    set(target general_functions_${component_name})
    add_library(${target} STATIC)
    add_library(GeneralFunctions::${component_name} ALIAS ${target})
    set_target_properties(${target} PROPERTIES
        EXPORT_NAME ${component_name}
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON)
    target_compile_features(${target} PUBLIC cxx_std_${cpp_standard})
    target_include_directories(${target} PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/generalFunctions>")
endfunction()

if(GENERAL_FUNCTIONS_BUILD_LEXER)
    add_general_functions_component(lexer 17)
    target_sources(general_functions_lexer PRIVATE lexer/lexer.cpp lexer/token_stream.cpp)
    install(TARGETS general_functions_lexer EXPORT GeneralFunctionsLexerTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES lexer/lexer.h lexer/token.h lexer/token_stream.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/generalFunctions/lexer)
    install(EXPORT GeneralFunctionsLexerTargets
        FILE GeneralFunctions-lexer-targets.cmake
        NAMESPACE GeneralFunctions::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeneralFunctions)
endif()

if(GENERAL_FUNCTIONS_BUILD_FILES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/file.cpp" OR
       NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/file.h")
        message(FATAL_ERROR
            "The files component requires file.cpp and file.h in this revision. "
            "Select a revision containing them or disable GENERAL_FUNCTIONS_BUILD_FILES.")
    endif()
    add_general_functions_component(files 17)
    target_sources(general_functions_files PRIVATE file.cpp)
    install(TARGETS general_functions_files EXPORT GeneralFunctionsFilesTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES file.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/generalFunctions)
    install(EXPORT GeneralFunctionsFilesTargets
        FILE GeneralFunctions-files-targets.cmake
        NAMESPACE GeneralFunctions::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeneralFunctions)
endif()

if(GENERAL_FUNCTIONS_BUILD_GEOMETRY)
    if(CMAKE_VERSION VERSION_LESS 3.28)
        message(FATAL_ERROR
            "The geometry module requires CMake >= 3.28. "
            "To build only lexer/files, set GENERAL_FUNCTIONS_BUILD_GEOMETRY=OFF.")
    endif()
    add_general_functions_component(geometry 20)
    target_sources(general_functions_geometry
        # Install/export the interface source so consumers can rebuild its BMI.
        PUBLIC FILE_SET CXX_MODULES FILES geometry/geometry.cpp
        PRIVATE geometry/geometry_impl.cpp)
    install(TARGETS general_functions_geometry EXPORT GeneralFunctionsGeometryTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_DATADIR}/generalFunctions/modules)
    install(EXPORT GeneralFunctionsGeometryTargets
        FILE GeneralFunctions-geometry-targets.cmake
        NAMESPACE GeneralFunctions:: CXX_MODULES_DIRECTORY modules
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeneralFunctions)
endif()

if(GENERAL_FUNCTIONS_BUILD_TREE)
    add_library(general_functions_tree INTERFACE)
    add_library(GeneralFunctions::tree ALIAS general_functions_tree)
    set_target_properties(general_functions_tree PROPERTIES EXPORT_NAME tree)
    target_compile_features(general_functions_tree INTERFACE cxx_std_17)
    target_include_directories(general_functions_tree INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/generalFunctions>")
    install(TARGETS general_functions_tree EXPORT GeneralFunctionsTreeTargets)
    install(FILES tree/binary_tree.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/generalFunctions/tree)
    install(EXPORT GeneralFunctionsTreeTargets
        FILE GeneralFunctions-tree-targets.cmake
        NAMESPACE GeneralFunctions::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeneralFunctions)
endif()

configure_package_config_file(cmake/GeneralFunctionsConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/GeneralFunctionsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeneralFunctions)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/GeneralFunctionsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY ExactVersion)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/GeneralFunctionsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/GeneralFunctionsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GeneralFunctions)

if(GENERAL_FUNCTIONS_BUILD_TESTS)
    enable_testing()
    if(GENERAL_FUNCTIONS_BUILD_TREE)
        add_executable(tree_smoke tests/tree_smoke.cpp)
        target_link_libraries(tree_smoke PRIVATE GeneralFunctions::tree)
        add_test(NAME tree_smoke COMMAND tree_smoke)
    endif()
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
    if(GENERAL_FUNCTIONS_BUILD_LEXER)
        foreach(component lexer token_stream)
            add_executable(${component}_tests tests/${component}_test.cpp)
            target_link_libraries(${component}_tests PRIVATE GeneralFunctions::lexer GTest::gtest_main)
            gtest_discover_tests(${component}_tests)
        endforeach()
    endif()
    if(GENERAL_FUNCTIONS_BUILD_GEOMETRY)
        add_executable(geometry_smoke tests/geometry_smoke.cpp)
        set_property(TARGET geometry_smoke PROPERTY CXX_EXTENSIONS OFF)
        target_link_libraries(geometry_smoke PRIVATE GeneralFunctions::geometry GTest::gtest_main)
        gtest_discover_tests(geometry_smoke)
    endif()
endif()

