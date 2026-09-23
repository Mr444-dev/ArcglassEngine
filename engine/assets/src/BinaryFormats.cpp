#include "engine/assets/BinaryFormats.h"
#include <random>
#include <sstream>
#include <iomanip>

namespace Engine::Assets {

std::string AssetGUID::ToString() const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(16) << high
       << "-"
       << std::setw(16) << low;
    return ss.str();
}

AssetGUID AssetGUID::FromString(const std::string& str) {
    AssetGUID guid{};
    auto dashPos = str.find('-');
    if (dashPos != std::string::npos) {
        std::string highStr = str.substr(0, dashPos);
        std::string lowStr = str.substr(dashPos + 1);
        guid.high = std::stoull(highStr, nullptr, 16);
        guid.low = std::stoull(lowStr, nullptr, 16);
    }
    return guid;
}

AssetGUID AssetGUID::CreateRandom() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<Core::u64> dis;

    AssetGUID guid{};
    guid.high = dis(gen);
    guid.low = dis(gen);
    return guid;
}

AssetGUID AssetGUID::FromContent(const void* data, size_t size) {
    // 64-bit FNV-1a Hash for high and low parts
    const auto* bytes = static_cast<const Core::u8*>(data);
    Core::u64 h1 = 0xcbf29ce484222325ULL;
    Core::u64 h2 = 0x100000001b3ULL;

    for (size_t i = 0; i < size; ++i) {
        h1 ^= bytes[i];
        h1 *= 0x100000001b3ULL;
        h2 ^= bytes[size - 1 - i];
        h2 *= 0xcbf29ce484222325ULL;
    }

    return AssetGUID{h1, h2};
}

} // namespace Engine::Assets
