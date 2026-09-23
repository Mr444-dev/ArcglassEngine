add_library(Engine_Physics STATIC
    include/engine/physics/Physics.h
    include/engine/physics/PhysicsTypes.h
    include/engine/physics/ColliderComponent.h
    include/engine/physics/RigidBodyComponent.h
    include/engine/physics/Collision.h
    include/engine/physics/CharacterController.h
    include/engine/physics/PhysicsScene.h
    src/Physics.cpp
    src/Collision.cpp
    src/CharacterController.cpp
    src/PhysicsScene.cpp
)

target_include_directories(Engine_Physics PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(Engine_Physics PUBLIC Engine_Core Engine_World)
target_set_compiler_warnings(Engine_Physics)
