# Min CMake version
cmake_minimum_required(VERSION 3.18)
project(GenericModulesProject CXX)

# Set directory vars
set(INCLUDE_DIR "include")

message(STATUS "Building for Unix")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Main executable
file(GLOB SOURCES "*.cpp")
add_library(GenericModuleLib ${SOURCES})
add_library(GenericModuleLib ${SOURCES})

# Test executable
add_executable(GenericModuleTests
    ${SOURCE_TESTS_DIR}/GenericModulesTests.cpp
)

if(NOT TARGET BaseModuleLib)
    add_subdirectory(components/Base_Module)
endif()

add_subdirectory(components/kissfft)

# Include directories for main code and tests
target_include_directories(GenericModuleLib
    PRIVATE ${INCLUDE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include/nlohmann
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/plog/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/kissfft
)

# Library A which need B shall be listed before B
target_link_libraries(GenericModuleTests PRIVATE BaseModuleLib GenericModuleLib ChunkTypesLib kissfft::kissfft)

# Enable testing
enable_testing()

# Add tests to CTest
add_test(NAME GenericModuleTests COMMAND GenericModuleTests)
