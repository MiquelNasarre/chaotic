add_library(chaotic_d3d11 STATIC)

target_sources(chaotic_d3d11
    PRIVATE
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/Blender.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/ConstantBuffer.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/DepthStencil.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/IndexBuffer.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/InputLayout.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/PixelShader.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/Rasterizer.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/Sampler.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/Texture.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/Topology.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/VertexBuffer.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Bindable/VertexShader.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Drawable/Background.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Drawable/Curve.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Drawable/Light.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Drawable/Polyhedron.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Drawable/Scatter.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Drawable/Surface.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/imgui/imgui.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/imgui/imgui_demo.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/imgui/imgui_draw.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/imgui/imgui_impl_dx11.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/imgui/imgui_impl_win32.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/imgui/imgui_tables.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/imgui/imgui_widgets.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/chaotic_demo.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Drawable.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/embedded_resources.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Error/DxgiInfoManager.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Graphics.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/iGManager.cpp"
        "${CHAOTIC_ROOT_DIR}/chaotic/source/Window.cpp"
)

target_include_directories(chaotic_d3d11
    PUBLIC
        "$<BUILD_INTERFACE:${CHAOTIC_ROOT_DIR}/chaotic/include>"
        "$<BUILD_INTERFACE:${CHAOTIC_ROOT_DIR}/chaotic/include/imgui>"
        "$<BUILD_INTERFACE:${CHAOTIC_ROOT_DIR}/chaotic/chaotic_headers>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_compile_features(chaotic_d3d11 PUBLIC cxx_std_20)
target_link_libraries(chaotic_d3d11 PUBLIC chaotic_core)

target_compile_definitions(chaotic_d3d11
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        "SOLUTION_DIR=\"${CHAOTIC_ROOT_DIR}/\""
)

target_link_libraries(chaotic_d3d11
    PRIVATE
        d3d11
        dxgi
        d3dcompiler
        dxguid
        dwmapi
        winmm
        user32
        kernel32
)
