add_library(Engine_World STATIC
    include/engine/world/World.h
    include/engine/world/Entity.h
    include/engine/world/Components.h
    include/engine/world/SparseSet.h
    include/engine/world/Registry.h
    include/engine/world/TransformHierarchy.h
    include/engine/world/SpatialBVH.h
    include/engine/world/Scene.h
    src/World.cpp
    src/TransformHierarchy.cpp
    src/SpatialBVH.cpp
    src/Scene.cpp
)

target_include_directories(Engine_World 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Engine_World PUBLIC Engine_Core Engine_Renderer)
target_set_compiler_warnings(Engine_World)
