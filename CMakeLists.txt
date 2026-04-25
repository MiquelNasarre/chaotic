cmake_minimum_required(VERSION 3.21)

project(chaotic LANGUAGES CXX)

set(CHAOTIC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
    set(CHAOTIC_BUILD_D3D11_BACKEND_DEFAULT ON)
else()
    set(CHAOTIC_BUILD_D3D11_BACKEND_DEFAULT OFF)
endif()

if(APPLE)
    set(CHAOTIC_BUILD_METAL_BACKEND_DEFAULT ON)
else()
    set(CHAOTIC_BUILD_METAL_BACKEND_DEFAULT OFF)
endif()

option(
    CHAOTIC_BUILD_D3D11_BACKEND
    "Build the Win32/Direct3D 11 backend target."
    ${CHAOTIC_BUILD_D3D11_BACKEND_DEFAULT}
)

option(
    CHAOTIC_BUILD_METAL_BACKEND
    "Build the macOS Metal backend target."
    ${CHAOTIC_BUILD_METAL_BACKEND_DEFAULT}
)

add_subdirectory(chaotic/core)

if(CHAOTIC_BUILD_D3D11_BACKEND)
    if(NOT WIN32)
        message(FATAL_ERROR "backend_d3d11 can only be built on Windows.")
    endif()

    add_subdirectory(chaotic/backend_d3d11)
endif()

if(CHAOTIC_BUILD_METAL_BACKEND)
    if(NOT APPLE)
        message(FATAL_ERROR "backend_metal can only be built on Apple platforms.")
    endif()

    add_subdirectory(chaotic/backend_metal)
endif()
