# CMakeList.txt: progetto CMake per AppG. Includere l'origine e definire
# qui la logica specifica del progetto.
#
cmake_minimum_required(VERSION 3.15)
# Aggiungere l'origine all'eseguibile di questo progetto.
# Per ora tolgo frontend e uso quickWindow, poi miglioriamo la struttura del progetto
add_executable (${APP_NAME} 
    "AppG.cpp" 
    "AppG.h" 
    "utils.h" 
    "utils.cpp" 
    "window/quickWindow.h" 
    "window/quickWindow.cpp"
    "app.manifest" #aggiunge il manifest, necessario per esempio per aware dpi
     "config.h" "init.h" "init.cpp" 
     "utils/wgpu_utils.h" "utils/wgpu_utils.cpp"
     "program.h" "program.cpp" "debug_helper.h"
     "programDetail.cpp" "common_def.h"
     "utils/quickShaders.h" "Engine/engine.h"
    "Engine/engine.cpp")

set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 20)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_precompile_headers(AppG PRIVATE "config.h" "debug_helper.h" "common_def.h")
#target_precompile_headers(AppG PRIVATE "config.h" "common_def.h")

set(Dawn_RELEASE_DIR "C:/Dev/libs/dawn_dist/lib/cmake/Dawn")
set(Dawn_DEBUG_DIR   "C:/Dev/libs/dawn_debug_dist/lib/cmake/Dawn")


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(Dawn_DIR "${Dawn_DEBUG_DIR}")
else()
    set(Dawn_DIR "${Dawn_RELEASE_DIR}")
endif()

find_package(Dawn REQUIRED)
target_link_libraries(AppG PRIVATE 
    dawn::webgpu_dawn 
    dawn::dawn_public_config
    Threads::Threads
    dxguid
    user32
    advapi32
    dbghelp
)
set (glfw3_DIR "C:/Dev/libs/glfw_dist/lib/cmake/glfw3")
find_package(glfw3 REQUIRED CONFIG)
target_link_libraries(AppG PRIVATE glfw)


set(DXC_DLL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extra")

#Trova vulkan dall'sdk

set(VULKAN_SDK_DIR $ENV{VULKAN_SDK})
message(STATUS "Vulkan SDK directory: ${VULKAN_SDK_DIR}")

#Aggiungere i .dll di directx e vulkan all'output
add_custom_command(TARGET AppG POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DXC_DLL_DIR}/dxcompiler.dll"
        "${DXC_DLL_DIR}/dxil.dll"
        "${DXC_DLL_DIR}/d3dcompiler_47.dll"
        "${DXC_DLL_DIR}/vulkan-1.dll"
        "$<TARGET_FILE_DIR:AppG>"
    COMMENT "Copia dei compilatori DirectX (DXC) nella cartella di build..."
)
