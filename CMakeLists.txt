cmake_minimum_required(VERSION 3.22)

# NOTE: update executable name in .github/workflows/cmake.yml:25 when changing executable name in this file
# for now, the project name is used as the executable name
set(MAIN_PROJECT_NAME "SoundFugue")
set(MAIN_EXECUTABLE_NAME "${MAIN_PROJECT_NAME}")

project(${MAIN_PROJECT_NAME})
enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/Options.cmake)
include(cmake/CompilerFlags.cmake)
include(cmake/CopyHelper.cmake)

###############################################################################
# 1. External Dependency: SFML 3 (FetchContent)
###############################################################################

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

set(SFML_WARNINGS_AS_ERRORS OFF)

# NOTE: Also update SFML_VERSION env var in .github/workflows/cmake.yml:33
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        # renovate: datasource=github-tags depName=SFML/SFML versioning=loose
        GIT_TAG        3.1.0

        GIT_SHALLOW    1
)

FetchContent_MakeAvailable(SFML)

###############################################################################
# 2. External Dependency: tmxlite (Local Source)
###############################################################################

# Define where the tmxlite external folder is
set(TMXLITE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ext/tmxlite")
set(TMXLITE_SRC_DIR "${TMXLITE_ROOT}/src")

# Combine the sources from the tmxlite CMake snippet you provided
set(TMXLITE_SOURCES
        ${TMXLITE_SRC_DIR}/FreeFuncs.cpp
        ${TMXLITE_SRC_DIR}/ImageLayer.cpp
        ${TMXLITE_SRC_DIR}/Map.cpp
        ${TMXLITE_SRC_DIR}/Object.cpp
        ${TMXLITE_SRC_DIR}/ObjectGroup.cpp
        ${TMXLITE_SRC_DIR}/Property.cpp
        ${TMXLITE_SRC_DIR}/TileLayer.cpp
        ${TMXLITE_SRC_DIR}/LayerGroup.cpp
        ${TMXLITE_SRC_DIR}/Tileset.cpp
        ${TMXLITE_SRC_DIR}/ObjectTypes.cpp
        # Lib sources (miniz, pugixml)
        ${TMXLITE_SRC_DIR}/miniz.c
        ${TMXLITE_SRC_DIR}/detail/pugixml.cpp
)

# Create the library target
add_library(tmxlite STATIC ${TMXLITE_SOURCES})

# Add the include directory so you can use #include <tmxlite/Map.hpp>
# This points to 'ext/tmxlite', which contains the 'tmxlite' folder with headers.
target_include_directories(tmxlite PUBLIC "${TMXLITE_ROOT}")

# (Optional) Ensure tmxlite knows where its own internal headers are if needed,
# though usually relative paths in source handle this.
target_include_directories(tmxlite PRIVATE "${TMXLITE_SRC_DIR}")


# Enable sanitizers for tmxlite (fixes MSVC LNK2038) but skip strict warnings
set_custom_stdlib_and_sanitizers(tmxlite TRUE)

###############################################################################
# System Dependencies
###############################################################################

find_package(Threads REQUIRED)
if(APPLE)
elseif(UNIX)
    find_package(X11)
endif()

###############################################################################
# Terrain Collision Library
###############################################################################

add_library(TerrainCollisionLib STATIC
        src/terrain/TerrainCollision.cpp
        src/terrain/TerrainCollision.hpp
        src/terrain/TerrainContacts.hpp
        src/terrain/TerrainMove.hpp
)
target_include_directories(TerrainCollisionLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(TerrainCollisionLib PUBLIC SFML::Graphics)
set_compiler_flags(RUN_SANITIZERS TRUE TARGET_NAMES TerrainCollisionLib)

###############################################################################
# Main Executable
###############################################################################

# NOTE: update executable name in .github/workflows/cmake.yml:25 when changing name here
add_executable(${MAIN_EXECUTABLE_NAME}
        src/main.cpp

        src/game/Game.cpp
        src/game/Game.hpp
        src/game/World.cpp
        src/game/World.hpp
        src/game/InputState.hpp
        src/game/Constants.hpp

        src/entities/Player.cpp
        src/entities/Player.hpp
        src/entities/Enemy.cpp
        src/entities/Enemy.hpp
        src/entities/enemies/Spider.cpp
        src/entities/enemies/Spider.hpp
        src/entities/Entity.cpp
        src/entities/Entity.hpp
        src/entities/Note.cpp
        src/entities/Note.hpp

        src/graphics/TextureHolder.cpp
        src/graphics/TextureHolder.hpp
        src/graphics/MapLayer.hpp
)

# NOTE: Add all defined targets (e.g. executables, libraries, etc. )
# NOTE: RUN_SANITIZERS is optional, if it's not present it will default to true
set_compiler_flags(RUN_SANITIZERS TRUE TARGET_NAMES ${MAIN_EXECUTABLE_NAME})

###############################################################################
# Linking and Include Configuration
###############################################################################

target_include_directories(${MAIN_EXECUTABLE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# use SYSTEM so cppcheck and clang-tidy do not report warnings from these directories
target_include_directories(${MAIN_EXECUTABLE_NAME} SYSTEM PRIVATE ${SFML_SOURCE_DIR}/include)
target_link_directories(${MAIN_EXECUTABLE_NAME} PRIVATE ${SFML_BINARY_DIR}/lib)

# Link SFML and the new tmxlite library
target_link_libraries(${MAIN_EXECUTABLE_NAME} PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        Threads::Threads
        tmxlite
        TerrainCollisionLib
)

if(APPLE)
elseif(UNIX)
    target_link_libraries(${MAIN_EXECUTABLE_NAME} PRIVATE X11)
endif()

# Test configuration
add_executable(TerrainCollisionTests
        tests/TerrainCollisionTests.cpp
)
target_link_libraries(TerrainCollisionTests PRIVATE TerrainCollisionLib)
set_compiler_flags(RUN_SANITIZERS TRUE TARGET_NAMES TerrainCollisionTests)
add_test(NAME TerrainCollision COMMAND TerrainCollisionTests)

###############################################################################
# Installation / Copying
###############################################################################

# copy binaries to "bin" folder; these are uploaded as artifacts on each release
# DESTINATION_DIR is set as "bin" in cmake/Options.cmake:6
install(TARGETS ${MAIN_EXECUTABLE_NAME} DESTINATION ${DESTINATION_DIR})
if(APPLE)
    install(FILES launcher.command DESTINATION ${DESTINATION_DIR})
endif()

copy_files(FILES tastatura.txt COPY_TO_DESTINATION TARGET_NAME ${MAIN_EXECUTABLE_NAME})
# copy_files(FILES tastatura.txt config.json DIRECTORY images sounds COPY_TO_DESTINATION TARGET_NAME ${MAIN_EXECUTABLE_NAME})
# copy_files(DIRECTORY images sounds COPY_TO_DESTINATION TARGET_NAME ${MAIN_EXECUTABLE_NAME})