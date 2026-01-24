cmake_minimum_required(VERSION 3.22.1)

project(TheEngineAndroid LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THEENGINE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../..)

add_library(TheEngine SHARED
    main_android.cpp
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    ${THEENGINE_ROOT}/DX3D/Include/Libraries/imgui-docking/imgui.cpp
    ${THEENGINE_ROOT}/DX3D/Include/Libraries/imgui-docking/imgui_draw.cpp
    ${THEENGINE_ROOT}/DX3D/Include/Libraries/imgui-docking/imgui_widgets.cpp
    ${THEENGINE_ROOT}/DX3D/Include/Libraries/imgui-docking/imgui_tables.cpp
    ${THEENGINE_ROOT}/DX3D/Include/Libraries/imgui-docking/backends/imgui_impl_android.cpp
    ${THEENGINE_ROOT}/DX3D/Include/Libraries/imgui-docking/backends/imgui_impl_opengl3.cpp
    ${THEENGINE_ROOT}/DX3D/Include/Libraries/stb/stb_image.cpp
)

file(GLOB_RECURSE ENGINE_SOURCES
    ${THEENGINE_ROOT}/DX3D/Source/DX3D/*.cpp
)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/Win32/")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/DirectX11/")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/OpenGL/")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/GraphicsDevice.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/DeviceContext.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/SwapChain.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/DirectWriteText.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/ShaderBinary.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/GraphicsPipelineState.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/VertexShaderSignature.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/VertexBuffer.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/IndexBuffer.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/ShadowMap.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/SkyboxRenderer.cpp$")

target_sources(TheEngine PRIVATE ${ENGINE_SOURCES})

target_include_directories(TheEngine PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${THEENGINE_ROOT}/DX3D/Include
    ${THEENGINE_ROOT}/DX3D/Source
    ${THEENGINE_ROOT}/DX3D/Include/Libraries/imgui-docking
    ${THEENGINE_ROOT}/DX3D/Include/Libraries/imgui-docking/backends
)

target_compile_definitions(TheEngine PRIVATE
    DX3D_PLATFORM_ANDROID
    IMGUI_IMPL_OPENGL_ES3
)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles-lib GLESv3)

target_link_libraries(TheEngine
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${gles-lib}
)
