add_library(Engine_Assets STATIC
    include/engine/assets/Assets.h
    include/engine/assets/BinaryFormats.h
    include/engine/assets/VirtualFileSystem.h
    include/engine/assets/AssetDatabase.h
    include/engine/assets/AssetCooker.h
    include/engine/assets/AsyncAssetLoader.h
    include/engine/assets/DirectStorageManager.h
    src/Assets.cpp
    src/BinaryFormats.cpp
    src/VirtualFileSystem.cpp
    src/AssetDatabase.cpp
    src/AssetCooker.cpp
    src/AsyncAssetLoader.cpp
    src/DirectStorageManager.cpp
)

target_include_directories(Engine_Assets 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Engine_Assets PUBLIC Engine_Core)
target_set_compiler_warnings(Engine_Assets)
