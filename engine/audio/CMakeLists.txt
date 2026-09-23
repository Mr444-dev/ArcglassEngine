add_library(Engine_Audio STATIC
    include/engine/audio/AudioTypes.h
    include/engine/audio/SpatialAudio.h
    include/engine/audio/AudioEngine.h
    src/SpatialAudio.cpp
    src/AudioEngine.cpp
)

target_include_directories(Engine_Audio 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Engine_Audio PUBLIC Engine_Core Engine_World)
target_set_compiler_warnings(Engine_Audio)
