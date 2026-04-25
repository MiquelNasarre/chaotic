enable_language(OBJCXX)

add_library(chaotic_metal STATIC)

target_sources(chaotic_metal
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source/MetalBackend.mm"
)

target_include_directories(chaotic_metal
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_compile_features(chaotic_metal PUBLIC cxx_std_20)
target_link_libraries(chaotic_metal PUBLIC chaotic_core)

target_compile_options(chaotic_metal
    PRIVATE
        "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>"
)

find_library(CHAOTIC_COCOA_FRAMEWORK Cocoa REQUIRED)
find_library(CHAOTIC_METAL_FRAMEWORK Metal REQUIRED)
find_library(CHAOTIC_QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)

target_link_libraries(chaotic_metal
    PRIVATE
        ${CHAOTIC_COCOA_FRAMEWORK}
        ${CHAOTIC_METAL_FRAMEWORK}
        ${CHAOTIC_QUARTZCORE_FRAMEWORK}
)

option(CHAOTIC_BUILD_METAL_DEMO "Build a small macOS demo executable for the Metal backend." ON)

if(CHAOTIC_BUILD_METAL_DEMO)
    add_executable(chaotic_metal_demo
        "${CMAKE_CURRENT_SOURCE_DIR}/source/MetalDemo.cpp"
    )

    target_compile_features(chaotic_metal_demo PRIVATE cxx_std_20)
    target_link_libraries(chaotic_metal_demo PRIVATE chaotic_metal)
endif()
