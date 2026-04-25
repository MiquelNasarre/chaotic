add_library(chaotic_core STATIC)

target_sources(chaotic_core
    PRIVATE
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Error/ChaoticError.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Image/Image.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Keyboard.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Math/Quaternion.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Math/Vectors.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Mouse.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Timer.cpp"
)

target_include_directories(chaotic_core
    PUBLIC
        "$<BUILD_INTERFACE:${CHAOTIC_ROOT_DIR}/chaotic/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_compile_features(chaotic_core PUBLIC cxx_std_20)
target_compile_definitions(chaotic_core PRIVATE CHAOTIC_CORE_ONLY)

if(MSVC)
    target_compile_definitions(chaotic_core PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(WIN32)
    target_link_libraries(chaotic_core PUBLIC user32 kernel32)
endif()
