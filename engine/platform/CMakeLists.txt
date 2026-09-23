add_library(Engine_Platform STATIC
    include/engine/platform/Platform.h
    include/engine/platform/Window.h
    include/engine/platform/HeadlessWindow.h
    include/engine/platform/Input.h
    src/Platform.cpp
    src/Window.cpp
    src/Input.cpp
)

target_include_directories(Engine_Platform PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(Engine_Platform PUBLIC Engine_Core)

if(WIN32)
    target_link_libraries(Engine_Platform PUBLIC user32 gdi32)
endif()

target_set_compiler_warnings(Engine_Platform)
