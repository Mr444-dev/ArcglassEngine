add_library(Engine_RHI STATIC
    include/engine/rhi/RHI.h
    include/engine/rhi/RHITypes.h
    include/engine/rhi/RHIDevice.h
    include/engine/rhi/RHIDescriptorHeap.h
    include/engine/rhi/RHISync.h
    include/engine/rhi/RHIBuffer.h
    include/engine/rhi/RHITexture.h
    include/engine/rhi/RHIPipelineState.h
    include/engine/rhi/RHICommandList.h
    include/engine/rhi/RHISwapChain.h
    include/engine/rhi/RHIShaderCompiler.h
    src/RHI.cpp
    src/d3d12/D3D12Common.h
    src/d3d12/D3D12DescriptorHeap.h
    src/d3d12/D3D12DescriptorHeap.cpp
    src/d3d12/D3D12Sync.h
    src/d3d12/D3D12Sync.cpp
    src/d3d12/D3D12Buffer.h
    src/d3d12/D3D12Buffer.cpp
    src/d3d12/D3D12Texture.h
    src/d3d12/D3D12Texture.cpp
    src/d3d12/D3D12PipelineState.h
    src/d3d12/D3D12PipelineState.cpp
    src/d3d12/D3D12CommandList.h
    src/d3d12/D3D12CommandList.cpp
    src/d3d12/D3D12SwapChain.h
    src/d3d12/D3D12SwapChain.cpp
    src/d3d12/D3D12Device.h
    src/d3d12/D3D12Device.cpp
    src/d3d12/D3D12ShaderCompiler.cpp
)

target_include_directories(Engine_RHI 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Engine_RHI PUBLIC Engine_Core Engine_Platform)

if(WIN32)
    target_link_libraries(Engine_RHI PUBLIC d3d12 dxgi dxguid d3dcompiler)
endif()

target_set_compiler_warnings(Engine_RHI)
