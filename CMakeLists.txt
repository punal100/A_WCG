cmake_minimum_required(VERSION 3.16)
project(A_WCG VERSION 1.0.0 LANGUAGES CXX)

# C++17 required for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Source files
set(SOURCES
    src/main.cpp
    src/awcg_html_parser.cpp
    src/awcg_css_parser.cpp
    src/awcg_element_mapper.cpp
    src/awcg_style_mapper.cpp
    src/awcg_json_generator.cpp
    src/awcg_cpp_generator.cpp
    src/awcg_html_preview.cpp
)

set(HEADERS
    src/awcg_types.h
    src/awcg_html_parser.h
    src/awcg_css_parser.h
    src/awcg_element_mapper.h
    src/awcg_style_mapper.h
    src/awcg_json_generator.h
    src/awcg_cpp_generator.h
    src/awcg_html_preview.h
)

# Executable
add_executable(awcg ${SOURCES} ${HEADERS})

# Platform-specific settings
if(WIN32)
    target_compile_definitions(awcg PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Enable warnings
if(MSVC)
    target_compile_options(awcg PRIVATE /W4)
else()
    target_compile_options(awcg PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install
install(TARGETS awcg RUNTIME DESTINATION bin)
