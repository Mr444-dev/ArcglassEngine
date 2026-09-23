add_library(Engine_Animation STATIC
    include/engine/animation/Animation.h
    include/engine/animation/Skeleton.h
    include/engine/animation/AnimationClip.h
    include/engine/animation/Animator.h
    include/engine/animation/SkinningPass.h
    src/Animation.cpp
    src/Skeleton.cpp
    src/AnimationClip.cpp
    src/Animator.cpp
    src/SkinningPass.cpp
)

target_include_directories(Engine_Animation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Engine_Animation PUBLIC Engine_Core Engine_RHI Engine_Renderer)
target_set_compiler_warnings(Engine_Animation)
