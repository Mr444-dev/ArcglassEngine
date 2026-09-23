#include "engine/core/Assert.h"
#include <iostream>

namespace Engine::Core {

void ReportAssertionFailure(
    std::string_view expression,
    std::string_view message,
    std::string_view file,
    u32 line
) {
    std::cerr << "\n=======================================================\n"
              << "[ASSERTION FAILED] \n"
              << "Expression: " << expression << "\n"
              << "Message:    " << message << "\n"
              << "File:       " << file << ":" << line << "\n"
              << "=======================================================\n"
              << std::endl;
}

} // namespace Engine::Core
