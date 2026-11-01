cmake_minimum_required(VERSION 3.24)
project(GeneralFunctionsConsumer LANGUAGES CXX)

option(TEST_GEOMETRY "Also consume the installed C++20 module" OFF)

find_package(GeneralFunctions CONFIG REQUIRED COMPONENTS lexer)

# Requesting lexer alone must not import the geometry module.
if(TARGET GeneralFunctions::geometry)
    message(FATAL_ERROR "Requesting lexer unexpectedly imported geometry")
endif()

add_executable(consumer ../lexer_smoke.cpp)
target_link_libraries(consumer PRIVATE GeneralFunctions::lexer)

if(TEST_GEOMETRY)
    find_package(GeneralFunctions CONFIG REQUIRED COMPONENTS geometry)

    add_executable(geometry_consumer ../geometry_smoke.cpp)
    set_target_properties(geometry_consumer PROPERTIES
        CXX_SCAN_FOR_MODULES ON
        CXX_EXTENSIONS OFF)
    target_link_libraries(geometry_consumer PRIVATE GeneralFunctions::geometry)
endif()

