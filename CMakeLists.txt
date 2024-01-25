cmake_minimum_required(VERSION 3.5)

project(asteroids)

add_executable(asteroids asteroids.c)

set_property(TARGET asteroids PROPERTY C_STANDARD 99)

# raylib
add_subdirectory(raylib)
target_include_directories(asteroids PRIVATE ${PROJECT_SOURCE_DIR}/raylib/src)
target_link_libraries(asteroids raylib)
