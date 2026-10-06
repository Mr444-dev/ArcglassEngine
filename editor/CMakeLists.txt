add_library(Engine_Editor_Lib STATIC
    include/editor/EditorContext.h
    include/editor/EditorPanel.h
    include/editor/GizmoController.h
    include/editor/EditorApp.h
    include/editor/panels/SceneHierarchyPanel.h
    include/editor/panels/PropertiesPanel.h
    include/editor/panels/AssetBrowserPanel.h
    include/editor/panels/ViewportPanel.h
    include/editor/panels/TelemetryPanel.h
    src/GizmoController.cpp
    src/EditorApp.cpp
    src/panels/SceneHierarchyPanel.cpp
    src/panels/PropertiesPanel.cpp
    src/panels/AssetBrowserPanel.cpp
    src/panels/ViewportPanel.cpp
    src/panels/TelemetryPanel.cpp
)

target_include_directories(Engine_Editor_Lib 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Engine_Editor_Lib PUBLIC
    Engine_Core
    Engine_Platform
    Engine_RHI
    Engine_World
    Engine_Assets
    Engine_Renderer
    Engine_Physics
    Engine_Animation
    Engine_Audio
)

target_set_compiler_warnings(Engine_Editor_Lib)

add_executable(EngineEditor src/Main.cpp ${CMAKE_BINARY_DIR}/ArcglassResources.rc)
target_link_libraries(EngineEditor PRIVATE Engine_Editor_Lib)
if(WIN32)
    target_link_libraries(EngineEditor PRIVATE comdlg32 shell32)
endif()
target_set_compiler_warnings(EngineEditor)

engine_compile_shader_set(EngineEditor shaders/editor/EditorUI.hlsl editor)
