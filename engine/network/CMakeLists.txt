add_library(Engine_Network STATIC
    src/ClientPredictionSystem.cpp
    src/ServerReplicationManager.cpp
)

target_include_directories(Engine_Network PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(Engine_Network PUBLIC 
    Engine_Core
)

target_set_compiler_warnings(Engine_Network)
