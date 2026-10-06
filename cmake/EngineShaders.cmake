if(WIN32)
    set(ENGINE_FXC_HINTS)
    if(DEFINED ENV{WindowsSdkVerBinPath})
        list(APPEND ENGINE_FXC_HINTS "$ENV{WindowsSdkVerBinPath}x64")
    endif()
    if(DEFINED ENV{WindowsSdkDir} AND DEFINED ENV{WindowsSDKVersion})
        list(APPEND ENGINE_FXC_HINTS "$ENV{WindowsSdkDir}bin/$ENV{WindowsSDKVersion}x64")
    endif()

    find_program(ENGINE_FXC_EXECUTABLE NAMES fxc.exe HINTS ${ENGINE_FXC_HINTS})
    if(NOT ENGINE_FXC_EXECUTABLE)
        message(FATAL_ERROR "fxc.exe was not found. Install the Windows SDK and configure from a Visual Studio developer environment.")
    endif()
endif()

function(engine_compile_shader_set TARGET_NAME SHADER_SOURCE SHADER_OUTPUT_FOLDER)
    if(NOT WIN32)
        return()
    endif()

    get_filename_component(SHADER_NAME "${SHADER_SOURCE}" NAME_WE)
    set(SOURCE_PATH "${CMAKE_SOURCE_DIR}/${SHADER_SOURCE}")
    set(OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_OUTPUT_FOLDER}")
    set(VS_BYTECODE "${OUTPUT_DIR}/${SHADER_NAME}.vs_5_1.cso")
    set(PS_BYTECODE "${OUTPUT_DIR}/${SHADER_NAME}.ps_5_1.cso")
    file(GLOB_RECURSE ENGINE_SHADER_INCLUDES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/shaders/*.hlsli")

    add_custom_command(
        OUTPUT "${VS_BYTECODE}" "${PS_BYTECODE}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
        COMMAND "${ENGINE_FXC_EXECUTABLE}" /nologo /E VSMain /T vs_5_1 /O3 /Fo "${VS_BYTECODE}" "${SOURCE_PATH}"
        COMMAND "${ENGINE_FXC_EXECUTABLE}" /nologo /E PSMain /T ps_5_1 /O3 /Fo "${PS_BYTECODE}" "${SOURCE_PATH}"
        DEPENDS "${SOURCE_PATH}" ${ENGINE_SHADER_INCLUDES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Compiling ${TARGET_NAME} HLSL shaders"
        VERBATIM
    )

    set(SHADER_TARGET "${TARGET_NAME}Shaders")
    add_custom_target(${SHADER_TARGET} DEPENDS "${VS_BYTECODE}" "${PS_BYTECODE}")
    add_dependencies(${TARGET_NAME} ${SHADER_TARGET})
endfunction()
