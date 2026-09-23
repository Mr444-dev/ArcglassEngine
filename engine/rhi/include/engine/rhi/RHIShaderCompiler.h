#pragma once

#include "engine/core/BaseTypes.h"
#include <string>
#include <vector>

namespace Engine::RHI {

class RHIShaderCompiler {
public:
    static bool CompileFromFile(
        const std::string& filePath,
        const std::string& entryPoint,
        const std::string& targetProfile,
        std::vector<Core::u8>& outBytecode
    );

    static bool CompileFromSource(
        const std::string& sourceCode,
        const std::string& entryPoint,
        const std::string& targetProfile,
        std::vector<Core::u8>& outBytecode
    );
};

} // namespace Engine::RHI
