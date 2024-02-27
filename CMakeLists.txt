# Min CMake version
cmake_minimum_required(VERSION 3.18)
project(ProcessingServerProject CXX)

# Set directory vars
set(INCLUDE_DIR "include")

message(STATUS "Building for Unix")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# For audio interface
find_library(ALSA_LIBRARY asound)

# Add an executable target
add_executable(Processing_Server Processing_Server/main.cpp)

add_subdirectory(Processing_Server/components/Generic_Modules Generic_Modules)
add_subdirectory(Processing_Server/components/Linux_Modules Linux_Modules)

# Include directories for main code and tests
target_include_directories(Processing_Server
    PRIVATE ${INCLUDE_DIR}
    PRIVATE ${INCLUDE_TESTS_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Processing_Server/components/Linux_Modules/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Processing_Server/components/Generic_Modules/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Processing_Server/components/Generic_Modules/components/Base_Module/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Processing_Server/components/Generic_Modules/components/Base_Module/components/Chunk_Types/
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Processing_Server/components/Generic_Modules/components/Base_Module/components/Chunk_Types/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Processing_Server/components/Generic_Modules/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Processing_Server/components/Generic_Modules/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include/nlohmann
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Processing_Server/components/Generic_Modules/components/Base_Module/components/plog/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Processing_Server/components/Generic_Modules/components/kissfft
)

target_link_libraries(Processing_Server PRIVATE GenericModuleLib LinuxModulesLib BaseModuleLib ChunkTypesLib kissfft::kissfft asound)