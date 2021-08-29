cmake_minimum_required(VERSION 3.0.0)
project(smolsoft3d VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 CONFIG REQUIRED)
find_package(sdl2-image CONFIG REQUIRED)
find_package(GLM CONFIG REQUIRED)

add_compile_definitions(GLM_FORCE_LEFT_HANDED)

add_executable(smolsoft3d WIN32 "source/main.cpp" "source/sdl_extra.hpp")
target_link_libraries(smolsoft3d PUBLIC SDL2::SDL2 SDL2::SDL2main SDL2::SDL2_image)

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)
