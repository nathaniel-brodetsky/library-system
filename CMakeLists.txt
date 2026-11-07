cmake_minimum_required(VERSION 3.20)

project(LibrarySystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LIBRARY_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(library_core
    src/date.cpp
    src/registries.cpp
    src/library_service.cpp
)
add_library(library::core ALIAS library_core)
target_include_directories(library_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(library_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(library_core PRIVATE /W4 /utf-8)
    if(LIBRARY_WARNINGS_AS_ERRORS)
        target_compile_options(library_core PRIVATE /WX)
    endif()
else()
    target_compile_options(library_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    if(LIBRARY_WARNINGS_AS_ERRORS)
        target_compile_options(library_core PRIVATE -Werror)
    endif()
endif()

add_executable(library_app src/main.cpp)
target_link_libraries(library_app PRIVATE library::core)
if(MSVC)
    target_compile_options(library_app PRIVATE /utf-8)
endif()
