add_executable(GameRuntime
    src/Main.cpp
)

target_link_libraries(GameRuntime PRIVATE
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

target_set_compiler_warnings(GameRuntime)
