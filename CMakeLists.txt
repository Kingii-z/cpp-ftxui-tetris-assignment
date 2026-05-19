cmake_minimum_required(VERSION 3.16)
project(tetris_game)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)

add_subdirectory(FTXUI)

add_executable(tetris main.cpp)


target_link_libraries(tetris PRIVATE ftxui::component ftxui::dom ftxui::screen)

if(WIN32)
    target_link_libraries(tetris PRIVATE user32)
endif()
