#include "engine/rhi/include/engine/rhi/RHIShaderCompiler.h"
#include "engine/core/Log.h"

#include <d3dcompiler.h>
#include <fstream>
#include <sstream>

namespace Engine::RHI {

bool RHIShaderCompiler::CompileFromFile(
    const std::string& filePath,
    const std::string& entryPoint,
    const std::string& targetProfile,
    std::vector<Core::u8>& outBytecode
) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        ENGINE_LOG_ERROR("Failed to open shader file: {}", filePath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return CompileFromSource(buffer.str(), entryPoint, targetProfile, outBytecode);
}

bool RHIShaderCompiler::CompileFromSource(
    const std::string& sourceCode,
    const std::string& entryPoint,
    const std::string& targetProfile,
    std::vector<Core::u8>& outBytecode
) {
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG) || !defined(NDEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(
        sourceCode.c_str(),
        sourceCode.length(),
        nullptr,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint.c_str(),
        targetProfile.c_str(),
        compileFlags,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            ENGINE_LOG_ERROR("HLSL Compilation Error ({} [{}]) : {}", 
                entryPoint, targetProfile, static_cast<const char*>(errorBlob->GetBufferPointer()));
            errorBlob->Release();
        }
        if (shaderBlob) shaderBlob->Release();
        return false;
    }

    if (shaderBlob) {
        const auto* data = static_cast<const Core::u8*>(shaderBlob->GetBufferPointer());
        const size_t size = shaderBlob->GetBufferSize();
        outBytecode.assign(data, data + size);
        shaderBlob->Release();
    }

    if (errorBlob) errorBlob->Release();
    return true;
}

} // namespace Engine::RHI
